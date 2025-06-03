#!/bin/bash
set -e

# Script to run Bespangle contract tests, using Docker for environment consistency.

DOCKER_IMAGE_NAME="bespangle-dev-env"
# Ensure we are in the script's directory to reliably find the project root
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJECT_ROOT=$(cd "$SCRIPT_DIR" && git rev-parse --show-toplevel) # More robust root finding

# --- Function Definitions ---

# Function to build contracts using the build script
build_contracts() {
    # Check if build should be skipped
    if [ "$SKIP_BUILD" = "1" ]; then
        echo "--- Skipping contract build (--skip-build flag set) ---"
        return 0
    fi
    
    echo "--- Building contracts using build.sh ---"
    
    # Ensure build script is executable
    chmod +x "$PROJECT_ROOT/build.sh"
    
    # Run build script with verbose output if requested
    if [ "$VERBOSE" = "1" ]; then
        "$PROJECT_ROOT/build.sh" --verbose
    else
        "$PROJECT_ROOT/build.sh"
    fi
    
    # Verify build was successful
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to build contracts" >&2
        exit 1
    fi
    
    echo "--- Contracts built successfully ---"
}

# Parse command line arguments
parse_args() {
    while [ $# -gt 0 ]; do
        case "$1" in
            --skip-build)
                export SKIP_BUILD=1
                shift
                ;;
            --verbose|-v)
                export VERBOSE=1
                shift
                ;;
            *)
                echo "Unknown option: $1"
                exit 1
                ;;
        esac
    done
}


# --- Main Script Logic ---

# Parse command line arguments
parse_args "$@"

# Check if running inside the designated Docker container
if [ "$BESPANGLE_IN_DOCKER" = "true" ]; then
    # --- INSIDE DOCKER --- #
    echo "--- Running tests inside Docker container --- "
    
    # Configure Git to use a writable config location and trust the workspace directories
    export GIT_CONFIG_GLOBAL="/tmp/gitconfig"
    git config --global --add safe.directory /workspace
    git config --global --add safe.directory /github/workspace
    
    # Suppress Git advice messages
    git config --global advice.detachedHead false
    
    # Also set the safe directory in the local config as a fallback
    if [ -d "/github/workspace/.git" ]; then
        cd /github/workspace
        git config --local --add safe.directory /github/workspace
        git config --local advice.detachedHead false
        cd - >/dev/null
    fi

    # Set paths relative to the workspace
    if [ -d "/github/workspace" ]; then
        # Running in GitHub Actions
        PROJECT_ROOT="/github/workspace"
    else
        # Running locally
        PROJECT_ROOT="/workspace"
    fi
    export PATH="/usr/opt/cdt/4.1.0/bin:$PATH"
    
    # Ensure build directory exists
    mkdir -p "${PROJECT_ROOT}/build"
    
    # Debug: Show build directory contents
    echo "--- Build directory contents ---"
    ls -la "${PROJECT_ROOT}/build" || true
    
    # Build all contracts using the build script
    cd "${PROJECT_ROOT}" || exit 1
    build_contracts

    # 1. Ensure eosio.token is built (silently)
    SYS_CONTRACTS_DIR="${PROJECT_ROOT}/build/eos-system-contracts"
    SYS_CONTRACTS_BUILD_DIR="${SYS_CONTRACTS_DIR}/build/contracts"
    TOKEN_WASM_PATH="${SYS_CONTRACTS_BUILD_DIR}/eosio.token/eosio.token.wasm"
    TOKEN_ABI_PATH="${SYS_CONTRACTS_BUILD_DIR}/eosio.token/eosio.token.abi"
    SYS_CONTRACTS_TAG=v3.8.0

    if [ ! -f "${TOKEN_WASM_PATH}" ]; then
        # Clean previous attempts if any
        rm -rf "${SYS_CONTRACTS_DIR}"
        mkdir -p "$(dirname "${SYS_CONTRACTS_DIR}")"

        # Clone and build eos-system-contracts silently
        git clone --quiet --depth 1 --branch ${SYS_CONTRACTS_TAG} \
            https://github.com/eosnetworkfoundation/eos-system-contracts.git "${SYS_CONTRACTS_DIR}"

        export CDT_ROOT=${CDT_ROOT}
        export PATH=${CDT_ROOT}/bin:$PATH
        mkdir -p "${SYS_CONTRACTS_DIR}/build"
        cd "${SYS_CONTRACTS_DIR}/build" || exit 1
        cmake ../ -DCMAKE_TOOLCHAIN_FILE=${CDT_ROOT}/lib/cmake/cdt/CDTWasmToolchain.cmake >/dev/null
        make -j$(nproc) >/dev/null 2>&1
        cd "${PROJECT_ROOT}" # Return to project root

        if [ ! -f "${TOKEN_WASM_PATH}" ]; then
           echo "ERROR: eosio.token build failed. WASM not found after build attempt." >&2
           exit 1
        fi
        if [ ! -f "${TOKEN_ABI_PATH}" ]; then
           echo "ERROR: eosio.token build failed. ABI not found after build attempt." >&2
           exit 1
        fi
    fi

    # Verify ABI exists (but don't print it)
    if [ ! -f "${TOKEN_ABI_PATH}" ]; then
        echo "ERROR: ABI file not found at ${TOKEN_ABI_PATH}" >&2
        exit 1
    fi


    # 4. Run the Mocha tests
    echo "--- Running Mocha tests --- "
    cd "${PROJECT_ROOT}/tests" || exit 1 # Ensure we are in the tests directory

    if [ ! -d "node_modules" ]; then
        echo "Node modules not found in tests/, running npm install..."
        npm install
    fi

    echo "--- Starting Mocha tests --- "
    npx mocha *.spec.js
    test_exit_code=$? # Capture exit code

    echo "--- Tests finished (inside Docker) --- "
    exit $test_exit_code # Exit Docker container with Mocha's exit code

else
    # --- OUTSIDE DOCKER --- #
    echo "--- Setting up Docker environment for tests --- "

    # 1. Build the Docker image (ensure it's up-to-date)
    echo "--- Building Docker image: ${DOCKER_IMAGE_NAME} (using Dockerfile at ${PROJECT_ROOT}) ---"
    # Use main Dockerfile at project root. Assume it includes build tools, node, etc.
    docker build -t "${DOCKER_IMAGE_NAME}" "${PROJECT_ROOT}"

    # 2. Run the tests inside the container
    echo "--- Running test script inside Docker container --- "
    
    # Build the command to pass through all arguments
    DOCKER_CMD="./run_tests.sh"
    
    # Add --skip-build if it was specified
    if [ "$SKIP_BUILD" = "1" ]; then
        DOCKER_CMD="$DOCKER_CMD --skip-build"
    fi
    
    # Add --verbose if it was specified
    if [ "$VERBOSE" = "1" ]; then
        DOCKER_CMD="$DOCKER_CMD --verbose"
    fi
    
    # Ensure the user running docker has permissions for the volume mount
    # Use --user to potentially avoid permission issues with generated files
    docker run --rm \
        --user "$(id -u):$(id -g)" \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace \
        -e BESPANGLE_IN_DOCKER=true \
        "${DOCKER_IMAGE_NAME}" \
        /bin/bash -c "$DOCKER_CMD"

    # Capture and report the exit code from the Docker container
    docker_exit_code=$?
    if [ $docker_exit_code -ne 0 ]; then
        echo "--- Tests failed inside Docker container (Exit code: $docker_exit_code) ---"
        exit $docker_exit_code
    else
        echo "--- Tests passed inside Docker container ---"
        exit 0
    fi
fi
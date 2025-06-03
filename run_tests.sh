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


# --- Main Script Logic ---

# Check if running inside the designated Docker container
if [ "$BESPANGLE_IN_DOCKER" = "true" ]; then
    # --- INSIDE DOCKER --- #
    echo "--- Running tests inside Docker container --- "

    # Set paths relative to /workspace
    PROJECT_ROOT="/workspace"
    export PATH="/usr/opt/cdt/4.1.0/bin:$PATH"
    
    # Ensure build directory exists
    mkdir -p "${PROJECT_ROOT}/build"
    
    # Build all contracts using the build script
    cd "${PROJECT_ROOT}" || exit 1
    build_contracts

    # 1. Ensure eosio.token is built
    SYS_CONTRACTS_DIR="${PROJECT_ROOT}/build/eos-system-contracts"
    SYS_CONTRACTS_BUILD_DIR="${SYS_CONTRACTS_DIR}/build/contracts"
    TOKEN_WASM_PATH="${SYS_CONTRACTS_BUILD_DIR}/eosio.token/eosio.token.wasm"
    TOKEN_ABI_PATH="${SYS_CONTRACTS_BUILD_DIR}/eosio.token/eosio.token.abi"
    SYS_CONTRACTS_TAG=v3.8.0

    if [ ! -f "${TOKEN_WASM_PATH}" ]; then
        echo "--- Building eosio.token (${SYS_CONTRACTS_TAG}) as artifacts not found at ${TOKEN_WASM_PATH} ---"

        # Clean previous attempts if any
        rm -rf "${SYS_CONTRACTS_DIR}"
        mkdir -p "$(dirname "${SYS_CONTRACTS_DIR}")"

        echo "Cloning eos-system-contracts tag ${SYS_CONTRACTS_TAG}..."
        git clone --depth 1 --branch ${SYS_CONTRACTS_TAG} https://github.com/eosnetworkfoundation/eos-system-contracts.git "${SYS_CONTRACTS_DIR}"

        echo "Configuring eosio.token build..."
        export CDT_ROOT=${CDT_ROOT} # Export the assumed CDT root
        export PATH=${CDT_ROOT}/bin:$PATH
        mkdir -p "${SYS_CONTRACTS_DIR}/build"
        cd "${SYS_CONTRACTS_DIR}/build" || exit 1
        cmake ../ -DCMAKE_TOOLCHAIN_FILE=${CDT_ROOT}/lib/cmake/cdt/CDTWasmToolchain.cmake

        echo "Compiling eosio.token..."
        make -j$(nproc)
        cd "${PROJECT_ROOT}" # Return to project root

        if [ ! -f "${TOKEN_WASM_PATH}" ]; then
           echo "ERROR: eosio.token build failed. WASM not found after build attempt."
           exit 1
        fi
        if [ ! -f "${TOKEN_ABI_PATH}" ]; then
           echo "ERROR: eosio.token build failed. ABI (${TOKEN_ABI_PATH}) not found after build attempt."
           exit 1
        fi
        echo "--- eosio.token build complete --- "
    else
        echo "--- Found existing eosio.token artifacts at ${TOKEN_WASM_PATH}, skipping build --- "
    fi

    # 2. Print ABI Contents
    if [ -f "${TOKEN_ABI_PATH}" ]; then
        echo "--- Contents of ${TOKEN_ABI_PATH} (inside Docker) ---"
        cat "${TOKEN_ABI_PATH}"
        echo "------------------------------------"
    else
        echo "WARN: ABI file not found at ${TOKEN_ABI_PATH} inside Docker."
        # Decide if this is critical - exit 1?
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
    # Ensure the user running docker has permissions for the volume mount
    # Use --user to potentially avoid permission issues with generated files
    docker run --rm \
        --user "$(id -u):$(id -g)" \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace \
        -e BESPANGLE_IN_DOCKER=true \
        "${DOCKER_IMAGE_NAME}" \
        ./run_tests.sh # Execute this same script inside the container

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
#!/bin/bash
set -e

# Script to run Bespangle contract tests, using Docker for environment consistency.

DOCKER_IMAGE_NAME="bespangle-dev-env"
# Ensure we are in the script's directory to reliably find the project root
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJECT_ROOT=$(cd "$SCRIPT_DIR" && git rev-parse --show-toplevel) # More robust root finding

# --- Function Definitions ---

# Function to build local contracts (runs inside Docker)
build_local_contracts() {
    echo "--- Dynamically building all contracts found in /workspace/org/src --- "
    chmod +x ./deploy.sh

    SRC_DIR="/workspace/org/src"
    if [ ! -d "${SRC_DIR}" ]; then
        echo "ERROR: Source directory ${SRC_DIR} not found. Cannot build contracts."
        exit 1
    fi

    # Function to build a single contract
    build_single_contract() {
        local cpp_file="$1"
        local contract_name="$(basename "$cpp_file" .cpp)"
        
        # Use the same build directory as deploy.sh
        local build_dir="${PROJECT_ROOT:-.}/build"
        if [ "$BESPANGLE_IN_DOCKER" = "true" ]; then
            build_dir="/workspace/build"  # Inside Docker container
        fi
        
        local wasm_file="${build_dir}/${contract_name}/${contract_name}.wasm"
        local abi_file="${build_dir}/${contract_name}/${contract_name}.abi"
        local hpp_file="${SRC_DIR}/${contract_name}.hpp"
        
        # Ensure build directory exists
        mkdir -p "${build_dir}/${contract_name}"
        
        echo "Building contract: $contract_name"
        echo "  Source: $cpp_file"
        echo "  Output: $wasm_file"

        # Skip tokenstaker and govweight contracts
        if [ "${contract_name}" = "tokenstaker" ] || [ "${contract_name}" = "govweight" ]; then
            echo "Skipping ${contract_name}, not building."
            return 0
        fi

        # Robust timestamp logic for .cpp/.hpp and .wasm/.abi (macOS/Linux)
        # Always get integer mtime or fail if not possible
        local cpp_mtime hpp_mtime wasm_mtime=0 abi_mtime=0 latest_src_mtime
        
        if ! cpp_mtime=$(stat -f "%m" "${cpp_file}" 2>/dev/null) && \
           ! cpp_mtime=$(stat -c "%Y" "${cpp_file}" 2>/dev/null); then
            echo "ERROR: Unable to get mtime for ${cpp_file}" >&2
            return 1
        fi

        if [ -f "$hpp_file" ]; then
            if ! hpp_mtime=$(stat -f "%m" "$hpp_file" 2>/dev/null) && \
               ! hpp_mtime=$(stat -c "%Y" "$hpp_file" 2>/dev/null); then
                echo "ERROR: Unable to get mtime for $hpp_file" >&2
                return 1
            fi
        else
            hpp_mtime=0
        fi

        if [ "$cpp_mtime" -gt "$hpp_mtime" ]; then
            latest_src_mtime=$cpp_mtime
        else
            latest_src_mtime=$hpp_mtime
        fi

        if [ -f "$wasm_file" ]; then
            if ! wasm_mtime=$(stat -f "%m" "$wasm_file" 2>/dev/null) && \
               ! wasm_mtime=$(stat -c "%Y" "$wasm_file" 2>/dev/null); then
                echo "ERROR: Unable to get mtime for $wasm_file" >&2
                return 1
            fi
        fi
        
        if [ -f "$abi_file" ]; then
            if ! abi_mtime=$(stat -f "%m" "$abi_file" 2>/dev/null) && \
               ! abi_mtime=$(stat -c "%Y" "$abi_file" 2>/dev/null); then
                echo "ERROR: Unable to get mtime for $abi_file" >&2
                return 1
            fi
        fi

        # Only build if missing or out-of-date
        echo "DEBUG: Checking contract $contract_name"
        echo "  cpp_mtime: $cpp_mtime"
        echo "  hpp_mtime: $hpp_mtime"
        echo "  latest_src_mtime: $latest_src_mtime"
        echo "  wasm_mtime: $wasm_mtime"
        echo "  abi_mtime: $abi_mtime"
        echo "  wasm exists? $( [ -f "$wasm_file" ] && echo yes || echo no )"
        echo "  abi exists? $( [ -f "$abi_file" ] && echo yes || echo no )"
        
        if [ ! -f "$wasm_file" ] || [ ! -f "$abi_file" ] || \
           [ "$latest_src_mtime" -gt "$wasm_mtime" ] || [ "$latest_src_mtime" -gt "$abi_mtime" ]; then
            echo "Building: ${contract_name}..."
            if ! build_output=$(./deploy.sh -a build -t "${contract_name}" 2>&1); then
                echo "-------------------------------------------------" >&2
                echo "ERROR: Build failed for contract ${contract_name} (exit code: $?)." >&2
                echo "Output:" >&2
                echo "$build_output" >&2
                echo "-------------------------------------------------" >&2
                return 1
            fi
        else
            echo "Skipping ${contract_name}, up-to-date."
        fi
        return 0
    }
    
    # Export the function so it's available in subshells
    export -f build_single_contract
    
    # Get the list of .cpp files and process them in parallel
    echo "Starting incremental parallel contract builds using $(nproc) processes..."
    
    # Create an array to store the PIDs of background processes
    pids=()
    
    # Process each .cpp file
    for cpp_file in "${SRC_DIR}"/*.cpp; do
        [ -e "$cpp_file" ] || continue  # handle case with no .cpp files
        
        # Run the build in a subshell and store the PID
        (
            if ! build_single_contract "$cpp_file"; then
                exit 1
            fi
        ) &
        pids+=($!)
    done
    
    # Wait for all background processes to complete and check their exit status
    local build_failed=0
    for pid in "${pids[@]}"; do
        if ! wait "$pid"; then
            build_failed=1
        fi
    done
    
    if [ "$build_failed" -ne 0 ]; then
        echo "ERROR: One or more parallel builds failed. See error details above. Exiting." >&2
        exit 1
    fi

    echo "--- Finished building local contracts --- "
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
    echo "--- Building all contracts ---"
    cd "${PROJECT_ROOT}" || exit 1
    
    # Run the build script
    if ! ./build.sh; then
        echo "ERROR: Failed to build contracts"
        exit 1
    fi
    
    echo "--- Contracts built successfully ---"

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

    # 3. Build local Bespangle contracts
    build_local_contracts

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
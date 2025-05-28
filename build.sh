#!/bin/bash

# Build Script for Bespangle Contracts
# This script handles building all or specific contracts
# MUST be run inside the Docker container

set -e

# If not running inside Docker, re-run inside Docker
if [ "$BESPANGLE_IN_DOCKER" != "true" ]; then
    # Get the script's directory and project root
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "$SCRIPT_DIR"
    
    # Build the command with all arguments
    CMD="./build.sh $@"
    
    # Run inside Docker
    exec ./docker-run.sh --command="$CMD"
    exit $?
fi

# Default values
CONTRACTS="all"
VERBOSE=0

# Function to check if a contract should be skipped
should_skip_contract() {
    local contract=$1
    
    # Load skip_contracts from network_config.json
    local skip_contracts=()
    if [ -f "network_config.json" ]; then
        # Use jq to safely extract the skip_contracts array
        if command -v jq &> /dev/null; then
            while IFS= read -r skip_contract; do
                skip_contracts+=("$skip_contract")
            done < <(jq -r '.skip_contracts[]?' network_config.json 2>/dev/null)
        fi
    fi
    
    # Also include hardcoded defaults for backward compatibility
    local default_skip_contracts=("tokenstaker" "govweight" "bountmanager")
    
    # Check against both sources
    for skip_contract in "${skip_contracts[@]}" "${default_skip_contracts[@]}"; do
        if [ "$contract" = "$skip_contract" ]; then
            echo "Skipping $contract (in skip list)"
            return 0  # Return true (0) if contract should be skipped
        fi
    done
    
    return 1  # Return false (1) if contract should be processed
}

# Function to build a single contract
build_contract() {
    local contract=$1
    local src_dir="org/src"
    local build_dir="build/${contract}"
    
    echo "Building $contract..."
    
    # Create build directory with proper permissions if it doesn't exist
    mkdir -p "$build_dir"
    chmod 775 "$build_dir" 2>/dev/null || true
    
    # Ensure the directory is owned by the current user
    chown -R $(id -u):$(id -g) "$build_dir" 2>/dev/null || true
    
    # Compile the contract using cdt-cpp from the Docker container
    if cdt-cpp -abigen \
        -I "${src_dir}" \
        -I "/workspace/org/include" \
        -I "/workspace/org/external" \
        -R "${src_dir}/ricardian" \
        -contract "${contract}" \
        -o "${build_dir}/${contract}.wasm" \
        "${src_dir}/${contract}.cpp"; then
        
        echo "Successfully built $contract"
        return 0
    else
        echo "ERROR: Failed to build $contract" >&2
        return 1
    fi
}

# Function to display usage information
print_usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -t, --target    Comma-separated list of contracts to build (default: all)"
    echo "  -v, --verbose   Enable verbose output"
    echo "  -h, --help      Display this help message"
}

# Main script execution
main() {
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -t|--target)
                CONTRACTS="$2"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE=1
                shift
                ;;
            -h|--help)
                print_usage
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
    done

    # Get list of contracts to process
    local contracts_to_process=()
    if [ "$CONTRACTS" = "all" ]; then
        # Find all .cpp files in the src directory
        for cpp_file in org/src/*.cpp; do
            [ -f "$cpp_file" ] || continue
            local contract=$(basename "$cpp_file" .cpp)
            if should_skip_contract "$contract"; then
                echo "Skipping contract: $contract (in SKIP_CONTRACTS)"
                continue
            fi
            contracts_to_process+=("$contract")
        done
    else
        # Process only specified contracts, still respecting SKIP_CONTRACTS
        IFS=',' read -ra requested_contracts <<< "$CONTRACTS"
        for contract in "${requested_contracts[@]}"; do
            if should_skip_contract "$contract"; then
                echo "Skipping contract: $contract (in SKIP_CONTRACTS)"
                continue
            fi
            contracts_to_process+=("$contract")
        done
    fi
    
    if [ ${#contracts_to_process[@]} -eq 0 ]; then
        echo "No contracts to build after applying filters"
        exit 0
    fi

    echo "Building contracts: ${contracts_to_process[*]}"
    for contract in "${contracts_to_process[@]}"; do
        if ! build_contract "$contract"; then
            echo "ERROR: Failed to build $contract" >&2
            exit 1
        fi
    done

    echo "All specified contracts built successfully"
}

# Run the main function
main "$@"

#!/bin/bash

# Automated Antelope Smart Contract Deployment Script
# This is a simplified version with basic parsing

set -e

# Default values
CONFIG_FILE="config.env"
ACCOUNTS_FILE="accounts/jungle4accounts.txt"
ACTION="both"
CONTRACTS="all"
VERBOSE=0

# List of contracts to skip
SKIP_CONTRACTS=("tokenstaker" "govweight" "bountmanager")

# Function to load accounts from accounts file
load_accounts() {
    local accounts_file="$1"
    if [ ! -f "$accounts_file" ]; then
        echo "Accounts file not found: $accounts_file"
        return 1
    fi
    
    if [ "$VERBOSE" -eq 1 ]; then
        echo "Loading accounts from $accounts_file..."
    fi
    
    # Source the accounts file to load variables
    # This is more compatible than using declare -g
    if [ "$VERBOSE" -eq 1 ]; then
        set -x
    fi
    . "$accounts_file"
    if [ "$VERBOSE" -eq 1 ]; then
        set +x
    fi
}

# Function to check if a contract should be skipped
should_skip_contract() {
    local contract=$1
    for skip_contract in "${SKIP_CONTRACTS[@]}"; do
        if [ "$contract" = "$skip_contract" ]; then
            return 0  # Return true (0) if contract should be skipped
        fi
    done
    return 1  # Return false (1) if contract should be processed
}

# Function to display usage information
print_usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -c, --config    Configuration file (default: config.env)"
    echo "  -f, --accounts  Accounts mapping file (default: accounts/jungle4accounts.txt)"
    echo "  -a, --action    Action to perform: build, deploy, or both (default: both)"
    echo "  -t, --target    Comma-separated list of contracts to target (default: all)"
    echo "  -v, --verbose   Enable verbose output"
    echo "  -h, --help      Display this help message"
}

# Function to build a contract
build_contract() {
    local contract=$1
    local src_dir="org/src"
    local build_dir="build/${contract}"
    
    echo "Building $contract..."
    
    # Create build directory if it doesn't exist
    mkdir -p "$build_dir"
    
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

# Function to deploy a contract
deploy_contract() {
    local contract=$1
    echo "Deploying $contract..."
    # Add your deploy command here
    # For example: cleos set contract $CONTRACT_ACCOUNT $PWD -p $CONTRACT_ACCOUNT@active
    return 0
}

# Main script execution
main() {
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -c|--config)
                CONFIG_FILE="$2"
                shift 2
                ;;
            -f|--accounts)
                ACCOUNTS_FILE="$2"
                shift 2
                ;;
            -a|--action)
                ACTION="$2"
                shift 2
                ;;
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

    # Check that the config file exists
    if [ ! -f "$CONFIG_FILE" ]; then
        echo "Error: Configuration file not found at '$CONFIG_FILE'"
        exit 2
    fi

    # Load accounts from the accounts file
    if [ -n "$ACCOUNTS_FILE" ]; then
        if [ ! -f "$ACCOUNTS_FILE" ]; then
            echo "Error: Accounts file not found: $ACCOUNTS_FILE"
            exit 1
        fi
        load_accounts "$ACCOUNTS_FILE"
    else
        echo "Error: No accounts file specified"
        exit 1
    fi

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
        echo "No contracts to process after applying filters"
        exit 0
    fi

    # Process the action
    case "$ACTION" in
        build)
            echo "Building contracts: ${contracts_to_process[*]}"
            for contract in "${contracts_to_process[@]}"; do
                if ! build_contract "$contract"; then
                    echo "ERROR: Failed to build $contract" >&2
                    exit 1
                fi
            done
            ;;
        deploy)
            echo "Deploying contracts: ${contracts_to_process[*]}"
            for contract in "${contracts_to_process[@]}"; do
                if ! deploy_contract "$contract"; then
                    echo "ERROR: Failed to deploy $contract" >&2
                    exit 1
                fi
            done
            ;;
        both)
            echo "Building and deploying contracts: ${contracts_to_process[*]}"
            for contract in "${contracts_to_process[@]}"; do
                if ! build_contract "$contract"; then
                    echo "ERROR: Failed to build $contract" >&2
                    exit 1
                fi
                if ! deploy_contract "$contract"; then
                    echo "ERROR: Failed to deploy $contract" >&2
                    exit 1
                fi
            done
            ;;
        *)
            echo "Error: Invalid action: $ACTION"
            print_usage
            exit 1
            ;;
    esac

    echo "Script completed successfully."
}

# Run the main function
main "$@"

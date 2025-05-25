#!/bin/bash

# Automated Antelope Smart Contract Deployment Script
# This script handles deployment of contracts, using build.sh for building
# MUST be run inside the Docker container

set -e

# If not running inside Docker, re-run inside Docker
if [ "$BESPANGLE_IN_DOCKER" != "true" ]; then
    # Get the script's directory and project root
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "$SCRIPT_DIR"
    
    # Build the command with all arguments
    CMD="./deploy.sh $@"
    
    # Run inside Docker
    exec ./docker-run.sh --command="$CMD"
    exit $?
fi

# Default values
CONFIG_FILE="config.env"
ACCOUNTS_FILE="accounts/jungle4accounts.txt"
ACTION="both"
CONTRACTS="all"
VERBOSE=0

# Path to build script
BUILD_SCRIPT="./build.sh"

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

# Function to build contracts using the build script
build_contracts() {
    local contracts="$1"
    local verbose_flag=""
    
    if [ "$VERBOSE" -eq 1 ]; then
        verbose_flag="-v"
    fi
    
    echo "Building contracts: $contracts"
    if ! $BUILD_SCRIPT -t "$contracts" $verbose_flag; then
        echo "ERROR: Failed to build contracts" >&2
        return 1
    fi
    return 0
}

# Function to deploy a contract
deploy_contract() {
    local contract=$1
    local build_dir="build/${contract}"
    local wasm_file="${build_dir}/${contract}.wasm"
    local abi_file="${build_dir}/${contract}.abi"
    
    echo "Deploying $contract..."
    
    # Verify the contract was built
    if [ ! -f "$wasm_file" ] || [ ! -f "$abi_file" ]; then
        echo "ERROR: Contract files not found for $contract" >&2
        echo "Please build the contract first using: ./build.sh -t $contract" >&2
        return 1
    fi
    
    # Get the account name for this contract from config
    local contract_account_var=$(echo "${contract}_account" | tr '[:lower:]' '[:upper:]')
    local contract_account=${!contract_account_var}
    
    if [ -z "$contract_account" ]; then
        echo "ERROR: No account configured for contract $contract" >&2
        echo "Please set ${contract_account_var} in your config file" >&2
        return 1
    fi
    
    # Deploy the contract using cleos
    echo "Deploying $contract to account $contract_account..."
    
    if [ "$VERBOSE" -eq 1 ]; then
        echo "Running: cleos set contract $contract_account $build_dir $wasm_file $abi_file -p $contract_account@active"
    fi
    
    if ! cleos set contract "$contract_account" "$build_dir" "$wasm_file" "$abi_file" -p "$contract_account@active"; then
        echo "ERROR: Failed to deploy $contract" >&2
        return 1
    fi
    
    echo "Successfully deployed $contract to $contract_account"
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

    # Process the action
    case "$ACTION" in
        build)
            if ! build_contracts "$CONTRACTS"; then
                exit 1
            fi
            ;;
        deploy)
            # Convert comma-separated list to array for processing
            IFS=',' read -ra contracts_to_deploy <<< "$CONTRACTS"
            for contract in "${contracts_to_deploy[@]}"; do
                if ! deploy_contract "$contract"; then
                    echo "ERROR: Failed to deploy $contract" >&2
                    exit 1
                fi
            done
            ;;
        both)
            if ! build_contracts "$CONTRACTS"; then
                exit 1
            fi
            # Convert comma-separated list to array for processing
            IFS=',' read -ra contracts_to_deploy <<< "$CONTRACTS"
            for contract in "${contracts_to_deploy[@]}"; do
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

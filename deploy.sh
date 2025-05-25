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

# Function to get network from accounts filename
get_network_from_file() {
    local filename=$(basename "$1")
    # Extract network name from filename (e.g., jungle4 from jungle4accounts.txt)
    local network="${filename%accounts.txt*}"
    # If no network prefix found, default to jungle4 for backward compatibility
    if [ -z "$network" ] || [ "$network" = "accounts" ]; then
        network="jungle4"
    fi
    echo "$network"
}

# Function to get endpoint for network
get_network_endpoint() {
    local network="$1"
    local config_file="network_config.json"
    
    # Check if jq is available
    if ! command -v jq &> /dev/null; then
        echo "ERROR: jq is required but not installed" >&2
        return 1
    fi
    
    if [ ! -f "$config_file" ]; then
        echo "ERROR: Network config file not found: $config_file" >&2
        return 1
    fi
    
    local endpoint=$(jq -r ".${network}.endpoint // empty" "$config_file" 2>/dev/null)
    if [ -z "$endpoint" ]; then
        echo "ERROR: No endpoint found for network: $network" >&2
        return 1
    fi
    
    echo "$endpoint"
}

# Function to load accounts from accounts file
load_accounts() {
    local accounts_file="$1"
    if [ ! -f "$accounts_file" ]; then
        echo "Accounts file not found: $accounts_file" >&2
        return 1
    fi
    
    # Set network from accounts filename
    NETWORK=$(get_network_from_file "$accounts_file")
    
    if [ "$VERBOSE" -eq 1 ]; then
        echo "Loading accounts from $accounts_file (network: $NETWORK)..."
    fi
    
    # Get network endpoint
    if ! NETWORK_ENDPOINT=$(get_network_endpoint "$NETWORK"); then
        return 1
    fi
    
    if [ "$VERBOSE" -eq 1 ]; then
        echo "Using network endpoint: $NETWORK_ENDPOINT"
    fi
    
    # Source the accounts file to load variables
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
    # Handle both local and container paths
    local build_dir="build/${contract}"
    # Check if we're in the container (where /workspace is the working dir)
    if [ -d "/workspace" ]; then
        build_dir="/workspace/${build_dir}"
    fi
    
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
    
    # Deploy the contract using cleos with the network endpoint
    echo "Deploying $contract to account $contract_account on $NETWORK..."
    
    local cleos_cmd="cleos -u $NETWORK_ENDPOINT set contract $contract_account $build_dir $wasm_file $abi_file -p $contract_account@active"
    
    if [ "$VERBOSE" -eq 1 ]; then
        echo "Running: $cleos_cmd"
    fi
    
    if ! $cleos_cmd; then
        echo "ERROR: Failed to deploy $contract to $NETWORK" >&2
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
            # Get list of contracts to deploy
            local contracts_to_deploy=()
            if [ "$CONTRACTS" = "all" ]; then
                # Find all .cpp files in the src directory (matching build.sh behavior)
                for cpp_file in org/src/*.cpp; do
                    [ -f "$cpp_file" ] || continue
                    local contract=$(basename "$cpp_file" .cpp)
                    if should_skip_contract "$contract"; then
                        echo "Skipping contract: $contract (in SKIP_CONTRACTS)"
                        continue
                    fi
                    contracts_to_deploy+=("$contract")
                done
            else
                # Process only specified contracts, still respecting SKIP_CONTRACTS
                IFS=',' read -ra requested_contracts <<< "$CONTRACTS"
                for contract in "${requested_contracts[@]}"; do
                    if should_skip_contract "$contract"; then
                        echo "Skipping contract: $contract (in SKIP_CONTRACTS)"
                        continue
                    fi
                    contracts_to_deploy+=("$contract")
                done
            fi
            
            if [ ${#contracts_to_deploy[@]} -eq 0 ]; then
                echo "No contracts to deploy after applying filters"
                exit 0
            fi
            
            echo "Deploying contracts: ${contracts_to_deploy[*]}"
            for contract in "${contracts_to_deploy[@]}"; do
                if ! deploy_contract "$contract"; then
                    echo "ERROR: Failed to deploy $contract" >&2
                    exit 1
                fi
            done
            ;;
            
        both)
            # First build all contracts
            if ! build_contracts "$CONTRACTS"; then
                exit 1
            fi
            
            # Then deploy using the same contract list logic as above
            local contracts_to_deploy=()
            if [ "$CONTRACTS" = "all" ]; then
                for cpp_file in org/src/*.cpp; do
                    [ -f "$cpp_file" ] || continue
                    local contract=$(basename "$cpp_file" .cpp)
                    if should_skip_contract "$contract"; then
                        echo "Skipping contract: $contract (in SKIP_CONTRACTS)"
                        continue
                    fi
                    contracts_to_deploy+=("$contract")
                done
            else
                IFS=',' read -ra requested_contracts <<< "$CONTRACTS"
                for contract in "${requested_contracts[@]}"; do
                    if should_skip_contract "$contract"; then
                        echo "Skipping contract: $contract (in SKIP_CONTRACTS)"
                        continue
                    fi
                    contracts_to_deploy+=("$contract")
                done
            fi
            
            if [ ${#contracts_to_deploy[@]} -eq 0 ]; then
                echo "No contracts to deploy after applying filters"
                exit 0
            fi
            
            echo "Deploying contracts: ${contracts_to_deploy[*]}"
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

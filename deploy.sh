#!/bin/bash

# Automated Antelope Smart Contract Deployment Script
# This script handles building and deploying contracts to a specified network
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
NETWORK_CONFIG="network_config.json"
ACTION="both"
CONTRACTS="all"
VERBOSE=0
NETWORK=""

# Path to build script
BUILD_SCRIPT="./build.sh"

# Function to display usage information
print_usage() {
    echo "Usage: $0 -n NETWORK [options]"
    echo "Options:"
    echo "  -n, --network NETWORK    Target network (e.g., jungle4, mainnet)"
    echo "  -c, --contracts CONTRACTS  Comma-separated list of contracts to deploy (default: all)"
    echo "  -a, --action ACTION      Action: build, deploy, or both (default: both)"
    echo "  -v, --verbose           Enable verbose output"
    echo "  -h, --help              Show this help message"
}

# Function to load network configuration
load_network_config() {
    local network="$1"
    
    # Check if jq is available
    if ! command -v jq &> /dev/null; then
        echo "ERROR: jq is required but not installed" >&2
        return 1
    fi
    
    if [ ! -f "$NETWORK_CONFIG" ]; then
        echo "ERROR: Network config file not found: $NETWORK_CONFIG" >&2
        return 1
    fi
    
    # Load network config
    local config_data
    if ! config_data=$(jq -r ".networks.${network} // empty" "$NETWORK_CONFIG" 2>/dev/null); then
        echo "ERROR: Failed to parse network config" >&2
        return 1
    fi
    
    if [ -z "$config_data" ]; then
        echo "ERROR: No configuration found for network: $network" >&2
        return 1
    fi
    
    # Export network config
    export NETWORK_ENDPOINT=$(echo "$config_data" | jq -r '.endpoint')
    export CHAIN_ID=$(echo "$config_data" | jq -r '.chain_id')
    export SYSTEM_SYMBOL=$(echo "$config_data" | jq -r '.system_symbol')
    export SYSTEM_CONTRACT=$(echo "$config_data" | jq -r '.system_contract')
    
    # Export accounts as environment variables
    local accounts=$(echo "$config_data" | jq -r '.accounts | to_entries[] | "export \(.key | ascii_upcase)=\"\(.value)\""')
    if [ -n "$accounts" ]; then
        eval "$accounts"
    fi
    
    if [ "$VERBOSE" -eq 1 ]; then
        echo "=== Network Configuration ==="
        echo "Network: $network"
        echo "Endpoint: $NETWORK_ENDPOINT"
        echo "Chain ID: $CHAIN_ID"
        echo "System Symbol: $SYSTEM_SYMBOL"
        echo "System Contract: $SYSTEM_CONTRACT"
        echo ""
        echo "=== Contract Accounts ==="
        echo "$accounts" | sed 's/export //g'
        echo "=========================="
    fi
}

# Function to check if a contract should be skipped
should_skip_contract() {
    local contract=$1
    for skip_contract in "${SKIP_CONTRACTS[@]-}"; do
        if [ "$contract" = "$skip_contract" ]; then
            return 0  # Return true (0) if contract should be skipped
        fi
    done
    return 1  # Return false (1) if contract should be processed
}

# Function to deploy a contract
deploy_contract() {
    local contract=$1
    
    # Get the account name for this contract from environment variables
    local account_var=$(echo "${contract}" | tr '[:lower:]' '[:upper:]')
    local account=${!account_var}
    
    if [ -z "$account" ]; then
        echo "ERROR: No account configured for contract: $contract" >&2
        return 1
    fi
    
    # Handle both local and container paths
    local build_dir="build/${contract}"
    if [ -d "/workspace" ]; then
        build_dir="/workspace/${build_dir}"
    fi
    
    local wasm_file="${build_dir}/${contract}.wasm"
    local abi_file="${build_dir}/${contract}.abi"
    
    echo "Deploying $contract..."
    
    # Verify the contract was built
    if [ ! -f "$wasm_file" ] || [ ! -f "$abi_file" ]; then
        echo "ERROR: Contract files not found for $contract" >&2
        echo "Please build the contract first using: ./build.sh --target $contract" >&2
        return 1
    fi
    
    # Deploy the contract using cleos with the network endpoint
    echo "Deploying $contract to account $account on $NETWORK..."
    
    local cleos_cmd="cleos -u $NETWORK_ENDPOINT set contract $account $build_dir $wasm_file $abi_file -p $account@active"
    
    if [ "$VERBOSE" -eq 1 ]; then
        echo "Running: $cleos_cmd"
    fi
    
    # Create a temporary file to capture command output
    local temp_output=$(mktemp)
    
    # Run the command and capture both stdout and stderr
    if ! $cleos_cmd > "$temp_output" 2>&1; then
        echo "ERROR: Failed to deploy $contract to $NETWORK" >&2
        echo "Command: $cleos_cmd" >&2
        echo "Output:" >&2
        cat "$temp_output" >&2
        rm -f "$temp_output"
        
        # Additional diagnostics
        echo "\nDiagnostic information:" >&2
        echo "- Network endpoint: $NETWORK_ENDPOINT" >&2
        echo "- Account: $account" >&2
        echo "- Build directory: $build_dir" >&2
        echo "- WASM file exists: $(if [ -f "$wasm_file" ]; then echo "yes"; else echo "no"; fi)" >&2
        echo "- ABI file exists: $(if [ -f "$abi_file" ]; then echo "yes"; else echo "no"; fi)" >&2
        
        # Check if keosd is running
        if ! pgrep -x "keosd" > /dev/null; then
            echo "- keosd is not running" >&2
        else
            echo "- keosd is running" >&2
        fi
        
        return 1
    fi
    
    # If we get here, the command succeeded - show the output if verbose
    if [ "$VERBOSE" -eq 1 ]; then
        cat "$temp_output"
    fi
    rm -f "$temp_output"
    
    echo "Successfully deployed $contract to $account"
    return 0
}

# Main script execution
main() {
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -n|--network)
                NETWORK="$2"
                shift 2
                ;;
            -c|--contracts)
                CONTRACTS="$2"
                shift 2
                ;;
            -a|--action)
                ACTION="$2"
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
                echo "ERROR: Unknown option: $1" >&2
                print_usage
                exit 1
                ;;
        esac
    done
    
    # Validate required parameters
    if [ -z "$NETWORK" ]; then
        echo "ERROR: Network must be specified with -n or --network" >&2
        print_usage
        exit 1
    fi
    
    # Load network configuration
    if ! load_network_config "$NETWORK"; then
        exit 1
    fi
    
    # Check current branch for informational purposes only
    local current_branch
    current_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
    echo "INFO: Deploying to $NETWORK from branch: $current_branch"
    
    # Determine which contracts to process
    local contracts_to_process=()
    if [ "$CONTRACTS" = "all" ]; then
        # Get all contract names from the accounts section
        contracts_to_process=($(jq -r ".networks.${NETWORK}.accounts | keys[]" "$NETWORK_CONFIG" 2>/dev/null))
        if [ ${#contracts_to_process[@]} -eq 0 ]; then
            echo "ERROR: No contracts found for network: $NETWORK" >&2
            exit 1
        fi
    else
        # Convert comma-separated list to array
        IFS=',' read -r -a contracts_to_process <<< "$CONTRACTS"
    fi
    
    # Process each contract
    for contract in "${contracts_to_process[@]}"; do
        contract=$(echo "$contract" | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]')
        
        # Check if contract should be skipped
        if should_skip_contract "$contract"; then
            echo "Skipping contract: $contract (in SKIP_CONTRACTS)"
            continue
        fi
        
        case "$ACTION" in
            build)
                echo "=== Building $contract ==="
                $BUILD_SCRIPT -t "$contract"
                ;;
            deploy)
                echo "=== Deploying $contract ==="
                deploy_contract "$contract"
                ;;
            both)
                echo "=== Building and Deploying $contract ==="
                if $BUILD_SCRIPT -t "$contract"; then
                    deploy_contract "$contract"
                else
                    echo "ERROR: Failed to build $contract" >&2
                    exit 1
                fi
                ;;
            *)
                echo "ERROR: Invalid action: $ACTION" >&2
                print_usage
                exit 1
                ;;
        esac
    done
    
    echo "=== Deployment to $NETWORK network complete! ==="
}

# Run the main function
main "$@"

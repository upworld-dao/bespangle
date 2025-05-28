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
    # Use jq with proper quoting to handle network names with special characters
    if ! config_data=$(jq -r --arg net "$network" '.networks[$net] // empty' "$NETWORK_CONFIG" 2>/dev/null); then
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
    
    # Get skip contracts from config
    local skip_contracts_json=$(jq -r '.skip_contracts // [] | join(" ")' "$NETWORK_CONFIG" 2>/dev/null)
    if [ -n "$skip_contracts_json" ]; then
        IFS=' ' read -r -a SKIP_CONTRACTS <<< "$skip_contracts_json"
        export SKIP_CONTRACTS
    fi
    
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
        if [ ${#SKIP_CONTRACTS[@]} -gt 0 ]; then
            echo -e "\n=== Skipping Contracts ==="
            printf "%s\n" "${SKIP_CONTRACTS[@]}"
        fi
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
        echo "Available accounts in config: $(jq -r ".networks.${NETWORK}.accounts | keys[]" "$NETWORK_CONFIG" 2>/dev/null | tr '\n' ' ')" >&2
        return 1
    fi
    
    echo "=== Deploying $contract to $account on $NETWORK ==="
    
    # Determine the correct build directory path
    local base_dir="."
    if [ -n "$GITHUB_WORKSPACE" ]; then
        base_dir="$GITHUB_WORKSPACE"
    fi
    
    local build_dir="${base_dir}/build/${contract}"
    local wasm_file="${build_dir}/${contract}.wasm"
    local abi_file="${build_dir}/${contract}.abi"
    
    # Ensure paths are absolute
    build_dir=$(realpath -m "$build_dir" 2>/dev/null || echo "$build_dir")
    wasm_file=$(realpath -m "$wasm_file" 2>/dev/null || echo "$wasm_file")
    abi_file=$(realpath -m "$abi_file" 2>/dev/null || echo "$abi_file")
    
    # Create build directory if it doesn't exist
    mkdir -p "$build_dir"
    chmod 777 "$build_dir" 2>/dev/null || true
    
    echo "=== Path Information ==="
    echo "Base directory: $base_dir"
    echo "Build directory: $build_dir"
    echo "WASM file: $wasm_file"
    echo "ABI file: $abi_file"
    echo "Current directory: $(pwd)"
    echo "Build directory contents:"
    ls -la "$build_dir" 2>&1 || echo "Cannot list directory"
    echo "======================="
    
    # Verify the contract was built
    if [ ! -f "$wasm_file" ] || [ ! -f "$abi_file" ]; then
        echo "ERROR: Contract files not found for $contract" >&2
        echo "  - Current directory: $(pwd)" >&2
        echo "  - Expected WASM: $wasm_file" >&2
        echo "  - Expected ABI: $abi_file" >&2
        echo "  - Build directory contents ($build_dir):" >&2
        ls -la "$build_dir" 2>&1 || echo "    (does not exist)" >&2
        echo "  - Build directory absolute path: $(realpath "$build_dir" 2>/dev/null || echo "not found")/" >&2
        echo "Please build the contract first using: ./build.sh --target $contract" >&2
        return 1
    fi
    
    # Verify the account exists on the blockchain
    if [ "$VERBOSE" -eq 1 ]; then
        echo "Verifying account $account exists on $NETWORK..."
    fi
    
    if ! cleos -u "$NETWORK_ENDPOINT" get account "$account" >/dev/null 2>&1; then
        echo "ERROR: Account $account does not exist on $NETWORK" >&2
        echo "Please create the account and fund it with resources before deployment." >&2
        return 1
    fi
    
    # Get the directory containing the wasm file
    local wasm_dir=$(dirname "$wasm_file")
    
    echo "=== Deploying with paths ==="
    echo "Contract directory: $wasm_dir"
    echo "WASM file: $wasm_file"
    echo "ABI file: $abi_file"
    
    # Function to buy RAM if needed
    buy_ram() {
        local payer=$1
        local receiver=$2
        local bytes=$3
        
        # Calculate EOS amount needed (1 KB = 0.1 EOS is a common rate, adjust as needed)
        local eos_amount=$(echo "scale=4; $bytes / 10000" | bc)
        # Ensure we buy at least 0.1 EOS worth of RAM
        eos_amount=$(echo "$eos_amount + 0.1" | bc)
        
        echo "💰 Attempting to buy $bytes bytes of RAM for $receiver (cost: ~$eos_amount EOS)..."
        
        # Format the amount with 4 decimal places
        local formatted_amount=$(printf "%.4f EOS" $eos_amount)
        
        echo "📝 Executing: cleos -u $NETWORK_ENDPOINT push action eosio buyram \"[$payer, $receiver, $formatted_amount]\" -p $payer@active"
        
        local output
        output=$(cleos -u "$NETWORK_ENDPOINT" push action eosio buyram "[\"$payer\",\"$receiver\",\"$formatted_amount\"]" -p "$payer@active" 2>&1)
        local status=$?
        
        if [ $status -ne 0 ]; then
            echo "❌ Failed to buy RAM. Error:"
            echo "$output"
            return 1
        else
            echo "✅ Successfully bought RAM. Transaction details:"
            echo "$output"
            return 0
        fi
    }
    
    # Deploy the contract using cleos with the network endpoint
    local max_retries=3
    local attempt=1
    local output
    local status
    
    while [ $attempt -le $max_retries ]; do
        echo "🚀 Deployment attempt $attempt of $max_retries..."
        
        if [ "$VERBOSE" -eq 1 ]; then
            echo "Running: cleos -u $NETWORK_ENDPOINT set contract $account $wasm_dir $wasm_file $abi_file -p $account@active"
        fi
        
        # Try to deploy the contract
        output=$(cleos -u "$NETWORK_ENDPOINT" --print-request --print-response \
            set contract "$account" "$wasm_dir" "$wasm_file" "$abi_file" \
            -p "$account@active" 2>&1)
        status=$?
        
        # Check for RAM error
        if echo "$output" | grep -qi "insufficient[[:space:]]*ram"; then
            echo "⚠️  Detected insufficient RAM error"
            
            # Extract the required RAM amount from the error message if possible
            local needed_ram=$(echo "$output" | grep -oP '(?i)needs\s+\K[0-9,]+' | tail -1 | tr -d ',')
            if [ -z "$needed_ram" ]; then
                needed_ram=400000  # Default to 400KB if we can't parse the needed amount
                echo "⚠️  Could not determine exact RAM needed, using default: $needed_ram bytes"
            else
                echo "🔍 Determined needed RAM: $needed_ram bytes"
                # Add 25% buffer to the needed RAM to ensure enough for table data
                needed_ram=$(($needed_ram * 125 / 100))
                echo "📊 Buying RAM with 25% buffer: $needed_ram bytes"
            fi
            
            # Try to buy more RAM
            if ! buy_ram "$account" "$account" "$needed_ram"; then
                echo "❌ Failed to buy RAM. Please manually add RAM to the account and try again."
                return 1
            fi
            
            # Wait a bit for the transaction to be processed
            echo "⏳ Waiting for RAM allocation to complete..."
            sleep 3
        elif [ $status -eq 0 ]; then
            echo "✅ Contract deployed successfully!"
            return 0
        else
            echo "❌ Deployment failed with unknown error:"
            echo "$output"
            
            # If we have retries left, wait a bit before trying again
            if [ $attempt -lt $max_retries ]; then
                local wait_time=$((attempt * 2))
                echo "⏳ Waiting $wait_time seconds before retry..."
                sleep $wait_time
            fi
        fi
        
        attempt=$((attempt + 1))
    done
    
    echo "❌ All $max_retries deployment attempts failed. Please check the account's resources and try again."
    return 1
    local start_time=$(date +%s)
    
    # Run the command and capture both stdout and stderr
    echo -n "Deploying $contract... "
    if ! "${cleos_cmd[@]}" > "$temp_output" 2>&1; then
        echo "❌ FAILED"
        echo "ERROR: Failed to deploy $contract to $NETWORK" >&2
        echo "Command: ${cleos_cmd[*]}" >&2
        echo -e "\n=== Error Output ===" >&2
        cat "$temp_output" >&2
        
        # Additional diagnostics
        echo -e "\n=== Diagnostic Information ===" >&2
        echo "- Network: $NETWORK" >&2
        echo "- Endpoint: $NETWORK_ENDPOINT" >&2
        echo "- Contract: $contract" >&2
        echo "- Account: $account" >&2
        echo "- Build Directory: $build_dir" >&2
        echo "- WASM File: $wasm_file $(if [ -f "$wasm_file" ]; then echo "✅"; else echo "❌ (MISSING)"; fi)" >&2
        echo "- ABI File: $abi_file $(if [ -f "$abi_file" ]; then echo "✅"; else echo "❌ (MISSING)"; fi)" >&2
        
        # Check wallet status
        echo -e "\n=== Wallet Status ===" >&2
        if ! cleos wallet list 2>&1; then
            echo "- keosd is not running or not accessible" >&2
        else
            echo -e "\n=== Wallet Keys ===" >&2
            cleos wallet keys 2>&1 || true
        fi
        
        rm -f "$temp_output"
        return 1
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # If we get here, the command succeeded
    if [ "$VERBOSE" -eq 1 ]; then
        echo -e "✅ SUCCESS (${duration}s)\n"
        echo "=== Deployment Output ==="
        cat "$temp_output"
        echo -e "\n=========================="
    else
        echo "✅ SUCCESS (${duration}s)"
    fi
    
    rm -f "$temp_output"
    return 0
}

# Function to set up the wallet
setup_wallet() {
    echo "Setting up wallet..."
    
    # Create wallet directory and set permissions
    mkdir -p ~/eosio-wallet
    chmod 700 ~/eosio-wallet
    
    # Create log directory and set permissions
    mkdir -p ~/eosio-wallet/logs
    chmod 700 ~/eosio-wallet/logs
    
    # Start keosd in the background with log file in the wallet directory
    echo "Starting keosd..."
    keosd --http-server-address=127.0.0.1:8900 > ~/eosio-wallet/logs/keosd.log 2>&1 &
    KESOD_PID=$!
    
    # Give keosd time to start
    echo "Waiting for keosd to start..."
    sleep 5
    
    # Check if keosd is running
    if ! ps -p $KESOD_PID > /dev/null; then
        echo "ERROR: Failed to start keosd"
        echo "=== keosd log ==="
        cat ~/eosio-wallet/logs/keosd.log 2>/dev/null || echo "No log file found"
        echo "================="
        exit 1
    fi
    
    # Create a new wallet
    echo "Creating new wallet..."
    WALLET_CREATE_OUTPUT=$(cleos wallet create --to-console 2>&1)
    
    # Extract password from the output
    WALLET_PASSWORD=$(echo "$WALLET_CREATE_OUTPUT" | grep -o 'PW5[^\"]*' | head -1)
    
    if [ -z "$WALLET_PASSWORD" ]; then
        echo "ERROR: Failed to extract wallet password"
        exit 1
    fi
    
    # Save the password for future use
    echo "$WALLET_PASSWORD" > ~/wallet_password.txt
    chmod 600 ~/wallet_password.txt
    
    # Import the private key if provided
    if [ -n "$DEPLOYER_PRIVATE_KEY" ]; then
        echo "Importing private key..."
        if ! echo "$DEPLOYER_PRIVATE_KEY" | cleos wallet import --private-key; then
            echo "WARNING: Failed to import private key (it might already be imported)"
        fi
    else
        echo "WARNING: No DEPLOYER_PRIVATE_KEY provided, wallet is empty"
    fi
}

# Function to clean up keosd process
cleanup_keosd() {
    if [ -n "$KESOD_PID" ]; then
        echo "Cleaning up keosd process..."
        kill $KESOD_PID 2>/dev/null || true
    fi
}

# Main script execution
main() {
    # Set up error handling
    trap cleanup_keosd EXIT
    
    # Set up the wallet
    setup_wallet
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
                ACTION="$(echo "$2" | tr '[:upper:]' '[:lower:]')"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE=1
                set -x  # Enable debug mode
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
    
    # Validate action parameter
    if [[ ! "$ACTION" =~ ^(build|deploy|both)$ ]]; then
        echo "ERROR: Invalid action: $ACTION. Must be one of: build, deploy, both" >&2
        print_usage
        exit 1
    fi
    
    # Check for required commands
    local required_commands=(jq cleos)
    for cmd in "${required_commands[@]}"; do
        if ! command -v "$cmd" &> /dev/null; then
            echo "ERROR: Required command not found: $cmd" >&2
            exit 1
        fi
    done
    
    echo -e "\n=== Starting Deployment ==="
    echo "Network: $NETWORK"
    echo "Action: $ACTION"
    
    # Load network configuration
    echo -e "\nLoading network configuration..."
    if ! load_network_config "$NETWORK"; then
        echo "ERROR: Failed to load configuration for network: $NETWORK" >&2
        exit 1
    fi
    
    # Display current branch for informational purposes
    local current_branch
    current_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
    echo -e "\nDeployment Details:"
    echo "- Branch: $current_branch"
    echo "- Network: $NETWORK"
    echo "- Endpoint: $NETWORK_ENDPOINT"
    echo "- Chain ID: $CHAIN_ID"
    
    # Determine which contracts to process
    local contracts_to_process=()
    if [ "$CONTRACTS" = "all" ]; then
        # Get all contract names from the accounts section
        echo -e "\nDiscovering contracts for network: $NETWORK"
        # Use jq with --arg to properly handle network names with special characters
        contracts_to_process=($(jq -r --arg net "$NETWORK" '.networks[$net].accounts | keys[]' "$NETWORK_CONFIG" 2>/dev/null))
        
        if [ ${#contracts_to_process[@]} -eq 0 ]; then
            echo "ERROR: No contracts configured for network: $NETWORK" >&2
            echo "Available networks: $(jq -r '.networks | keys | join(", ")' "$NETWORK_CONFIG" 2>/dev/null)" >&2
            exit 1
        fi
        
        if [ "$VERBOSE" -eq 1 ]; then
            echo "Found ${#contracts_to_process[@]} contracts to process"
        fi
    else
        # Convert comma-separated list to array
        IFS=',' read -r -a contracts_to_process <<< "$CONTRACTS"
        echo -e "\nProcessing specified contracts: ${contracts_to_process[*]}"
    fi
    
    # Create build directory with proper permissions if it doesn't exist
    if [ ! -d "build" ]; then
        echo "Creating build directory..."
        mkdir -p build || { echo "Failed to create build directory"; exit 1; }
        chmod 775 build || { echo "Failed to set permissions on build directory"; exit 1; }
    fi
    
    # Ensure the build directory is owned by the current user
    chown -R $(id -u):$(id -g) build 2>/dev/null || true
    
    # Process each contract
    local success_count=0
    local fail_count=0
    local skipped_count=0
    local start_time=$(date +%s)
    
    echo -e "\n=== Starting $ACTION process ==="
    
    for contract in "${contracts_to_process[@]}"; do
        contract=$(echo "$contract" | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]')
        
        # Check if contract should be skipped
        if should_skip_contract "$contract"; then
            echo -e "\n⚠️  Skipping contract: $contract (in SKIP_CONTRACTS)"
            ((skipped_count++))
            continue
        fi
        
        # Process based on action
        case "$ACTION" in
            build)
                echo -e "\n🔨 Building $contract..."
                if $BUILD_SCRIPT -t "$contract"; then
                    echo "✅ Successfully built $contract"
                    ((success_count++))
                else
                    echo "❌ Failed to build $contract" >&2
                    ((fail_count++))
                    [ "$VERBOSE" -eq 1 ] && continue
                    exit 1
                fi
                ;;
                
            deploy)
                if deploy_contract "$contract"; then
                    ((success_count++))
                else
                    ((fail_count++))
                    [ "$VERBOSE" -eq 1 ] && continue
                    exit 1
                fi
                ;;
                
            both)
                echo -e "\n🚀 Building and Deploying $contract"
                if $BUILD_SCRIPT -t "$contract"; then
                    echo "✅ Build successful, deploying..."
                    if deploy_contract "$contract"; then
                        ((success_count++))
                    else
                        ((fail_count++))
                        [ "$VERBOSE" -eq 1 ] && continue
                        exit 1
                    fi
                else
                    echo "❌ Build failed for $contract" >&2
                    ((fail_count++))
                    [ "$VERBOSE" -eq 1 ] && continue
                    exit 1
                fi
                ;;
        esac
    done
    
    # Calculate duration
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Print summary
    echo -e "\n=== Deployment Summary ==="
    echo "Network:        $NETWORK"
    echo "Action:         $ACTION"
    echo "Duration:       ${duration}s"
    echo "Total:          $((success_count + fail_count + skipped_count))"
    echo -n "✅ Success:      $success_count"
    [ $skipped_count -gt 0 ] && echo -n "   ⏩ Skipped: $skipped_count"
    [ $fail_count -gt 0 ] && echo -n "   ❌ Failed: $fail_count"
    echo -e "\n=========================="
    
    # Exit with error if any failures occurred
    if [ $fail_count -gt 0 ]; then
        echo "ERROR: $fail_count contract(s) failed to process" >&2
        exit 1
    fi
    
    echo -e "\n✨ Successfully completed $ACTION process for $success_count contract(s) on $NETWORK"
    exit 0
}

# Run the main function
main "$@"

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
RUN_TESTS=0
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
    echo "  --run-tests             Run tests after building and before deploying"
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
        # Use awk for floating point math to avoid bc dependency
        local eos_amount_calc_output
        eos_amount_calc_output=$(awk -v bytes="$bytes" 'BEGIN { printf "%.4f", (bytes / 10000) + 0.1 }' 2>&1)
        if [ $? -ne 0 ]; then
            echo "ERROR: awk failed calculating initial EOS amount for RAM for $contract. Input bytes: $bytes. Error: $eos_amount_calc_output" >&2
            return 1
        fi
        local eos_amount="$eos_amount_calc_output"

        local final_eos_amount_output
        final_eos_amount_output=$(awk -v amt="$eos_amount" 'BEGIN { printf "%.4f", amt < 1.0 ? 1.0 : amt }' 2>&1)
        if [ $? -ne 0 ]; then
            echo "ERROR: awk failed adjusting final EOS amount for RAM for $contract. Input amount: $eos_amount. Error: $final_eos_amount_output" >&2
            return 1
        fi
        eos_amount="$final_eos_amount_output"
        
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
    local deployment_success=0
    
    while [ $attempt -le $max_retries ]; do
        echo "🚀 Deployment attempt $attempt of $max_retries..."

        if [ "$VERBOSE" -eq 1 ]; then
            echo "Running: cleos -u $NETWORK_ENDPOINT set contract $account $wasm_dir $wasm_file $abi_file -p $account@active"
        fi
        
        # Try to deploy the contract
        output=$(cleos -u "$NETWORK_ENDPOINT" \
            set contract "$account" "$wasm_dir" "$wasm_file" "$abi_file" \
            -p "$account@active" 2>&1)
        status=$?

        # Print cleos output and fail if there was an error
        if [ $status -ne 0 ]; then
            # Propagate RAM errors and fail
            if echo "$output" | grep -Eqi "insufficient[[:space:]]*ram|ram_usage_exceeded|not enough ram|needs [0-9,]+ bytes|account does not have enough RAM|cannot create table"; then
                echo "❌ RAM error detected during contract deployment for $contract (account: $account)"
                echo "$output"
                return 1
            fi
            echo "❌ cleos set contract failed for $contract (account: $account)"
            echo "$output"
            return 1
        fi

        # Check if deployment was successful or if the code/ABI is the same
        if [ $status -eq 0 ] || echo "$output" | grep -qi "Skipping set \(code\|abi\) because the new \(code\|abi\) is the same as the existing"; then
            if [ $status -eq 0 ]; then
                echo "✅ Contract deployed successfully"
            else
                echo "ℹ️  Contract is already up to date"
                # This is not an error, so we should still return success
                return 0
            fi
            return 0
        fi

        # If we get here, there was an error
        echo "❌ Deployment failed with error:"
        echo "======================================"
        echo "Command: cleos -u $NETWORK_ENDPOINT --print-request --print-response set contract $account $build_dir $wasm_file $abi_file -p $account@active"
        echo "Exit status: $status"
        echo "----------------------------------------"
        echo "$output"
        echo "========================================"

        # Check for common errors and provide suggestions
        if echo "$output" | grep -qi "insufficient[[:space:]]*ram"; then
            echo "💡 Still insufficient RAM after attempt to buy RAM."
        elif echo "$output" | grep -qi "transaction[[:space:]]*net[[:space:]]*usage[[:space:]]*is[[:space:]]*too[[:space:]]*high"; then
            echo "💡 Transaction too large. Try deploying with fewer contracts at once."
            return 1
        elif echo "$output" | grep -qi "missing[[:space:]]*authority"; then
            echo "💡 Missing authority. Ensure the account has the correct permissions."
            echo "   Required permission: $account@active"
            return 1
        elif echo "$output" | grep -qi "unknown[[:space:]]*key"; then
            echo "💡 Unknown key error. Check if the account exists and the private key is correct."
            return 1
        elif echo "$output" | grep -qi "does not exist"; then
            echo "💡 Account does not exist. Please create the account first."
            return 1
        fi

        echo "🔄 Retrying deployment (attempt $((attempt + 1))/$MAX_ATTEMPTS)..."
        sleep 2
        attempt=$((attempt + 1))
    done
    
    # This function should never reach here due to the return statements in the while loop
    echo "❌ Reached unexpected code path in deploy_contract function" >&2
    return 1
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
    local exit_code=$1
    
    echo -e "\nCleaning up keosd process..."
    pkill -f "keosd" || true
    
    # Always return the original exit code
    return $exit_code
}

# Main script execution
main() {
    set +e  # Disable exit on error so errors are handled explicitly
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
                # set -x  # Debug mode disabled (was enabled here)
                shift
                ;;
            -h|--help)
                print_usage
                return 0
                ;;
            --run-tests)
                RUN_TESTS=1
                shift
                ;;
            *)
                echo "ERROR: Unknown option: $1" >&2
                print_usage
                return 1
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
            return 1
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
    local contracts_to_process
    if [ "$CONTRACTS" = "all" ]; then
        echo -e "\nDiscovering contracts for network: $NETWORK"
        mapfile -t contracts_to_process < <(jq -r --arg net "$NETWORK" '.networks[$net].accounts | keys[]' "$NETWORK_CONFIG" 2>/dev/null)
        echo "DEBUG: contracts_to_process = ${contracts_to_process[*]}"
        if [ ${#contracts_to_process[@]} -eq 0 ]; then
            echo "ERROR: No contracts configured for network: $NETWORK" >&2
            echo "Available networks: $(jq -r '.networks | keys | join(", ")' "$NETWORK_CONFIG" 2>/dev/null)" >&2
            return 1
        fi
        if [ "$VERBOSE" -eq 1 ]; then
            echo "Found ${#contracts_to_process[@]} contracts to process: ${contracts_to_process[*]}"
        fi
    else
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

    local success_count=0
    local fail_count=0
    local skipped_count=0
        local start_time=$(date +%s)
        
        echo -e "\n=== Starting $ACTION process ==="
        
        # First, build all contracts if needed
        if [ "$ACTION" = "both" ] || [ "$ACTION" = "build" ]; then
            echo -e "\n🔨 Building all contracts via './build.sh -t \"$CONTRACTS\"'..."
            ./build.sh -t "$CONTRACTS"
            local build_status=$?
            if [ $build_status -ne 0 ]; then
                echo "⚠️ Build script './build.sh -t \"$CONTRACTS\"' exited with status $build_status. Continuing with deployment attempts..." >&2
                # Do not set overall_status=1 here, allow deployment attempts for contracts that might have built before the failure.
            else
                echo "✅ Build script './build.sh -t \"$CONTRACTS\"' completed successfully."

                # Run tests if --run-tests is specified and build was successful
                if [ "$RUN_TESTS" -eq 1 ]; then
                    echo -e "\n--- Running tests ---"
                    local test_script_args="--skip-build"
                    if [ "$VERBOSE" -eq 1 ]; then
                        test_script_args="$test_script_args --verbose"
                    fi
                    
                    # Ensure run_tests.sh is executable
                    if [ -f ./run_tests.sh ]; then
                        chmod +x ./run_tests.sh
                    else
                        echo "ERROR: run_tests.sh not found in current directory ($(pwd))" >&2
                        cleanup_keosd
                        exit 1 # Critical error, tests cannot run
                    fi
                    
                    echo "Executing: ./run_tests.sh $test_script_args"
                    if ./run_tests.sh $test_script_args; then
                        echo "--- Tests passed successfully ---"
                    else
                        echo "ERROR: Tests failed. Aborting further operations." >&2
                        cleanup_keosd
                        exit 1 # Tests failed, stop everything
                    fi
                elif [ "$RUN_TESTS" -eq 1 ]; then
                     echo "WARNING: --run-tests specified but build was not successful. Skipping tests." >&2
                fi
            fi
        fi
        
        # If action was just 'build', we're done. 
        # If tests were run and passed, and action is 'build', this is also a valid exit point.
        if [ "$ACTION" = "build" ]; then
            echo "Build completed. Use 'deploy' action to deploy the contracts."
            return 0
        fi

        echo "[DEBUG] Proceeding to deployment phase for contracts: ${contracts_to_process[*]}"

    # Process each contract for deployment
    local deployment_output=""
    for contract in "${contracts_to_process[@]}"; do
        contract=$(echo "$contract" | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]')
        
        # Skip if in skip list
        if should_skip_contract "$contract"; then
            echo "Skipping $contract (in skip list)"
            ((skipped_count++))
            continue
        fi
        
        case "$ACTION" in
            deploy)
                echo -e "\n🚀 Deploying contract: $contract"
                output=$(deploy_contract "$contract" 2>&1)
                status=$?
                if [ $status -eq 0 ]; then
                    if echo "$output" | grep -q -E -i "Skipping set (code|abi) because the new (code|abi) is the same as the existing"; then
                        echo "✅ Contract already up to date: $contract"
                        deployment_output+="✅ Contract already up to date: $contract\n"
                        ((skipped_count++))
                    else
                        echo "✅ Successfully deployed: $contract"
                        deployment_output+="✅ Successfully deployed: $contract\n"
                        ((success_count++))
                    fi
                else
                    echo "❌ Deployment failed for $contract" >&2
                    echo "Error details:" >&2
                    echo "$output" >&2
                    deployment_output+="❌ Deployment failed for $contract\n"
                    ((fail_count++))
                fi
                ;;
                
            both)
                # Now deploy the current contract
                echo -e "\n🚀 Deploying contract: $contract"
                output=$(deploy_contract "$contract" 2>&1)
                status=$?
                if [ $status -eq 0 ]; then
                    if echo "$output" | grep -q -E -i "Skipping set (code|abi) because the new (code|abi) is the same as the existing"; then
                        echo "✅ Contract already up to date: $contract"
                        deployment_output+="✅ Contract already up to date: $contract\n"
                        ((skipped_count++))
                    else
                        echo "✅ Successfully deployed: $contract"
                        deployment_output+="✅ Successfully deployed: $contract\n"
                        ((success_count++))
                    fi
                else
                    echo "❌ Deployment failed for $contract after retries" >&2
                    echo "Error details:" >&2
                    echo "$output" >&2
                    deployment_output+="❌ Deployment failed for $contract\n"
                    ((fail_count++))
                fi
                ;;
        esac
    done
    
    # Calculate duration
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Print summary with color coding
    echo -e "\n=== Deployment Summary ==="
    echo "Network:        $NETWORK"
    echo "Action:         $ACTION"
    echo "Duration:       ${duration}s"
    echo "Total:          $((success_count + fail_count + skipped_count))"
    
    # Success count in green
    echo -n "✅ Success:    $success_count"
    
    # Skipped count in yellow if any
    if [ $skipped_count -gt 0 ]; then
        echo -n "   ⏩ Skipped: $skipped_count"
    fi
    
    # Failed count in red if any
    if [ $fail_count -gt 0 ]; then
        echo -n "   ❌ Failed: $fail_count"
    fi
    
    echo -e "\n=========================="
    
    # Print detailed failure information if any
    if [ $fail_count -gt 0 ]; then
        echo -e "\n🔍 Failed Contract Details:"
        echo "----------------------------------------"
        for contract in "${contracts_to_process[@]}"; do
            # Just list the contracts that failed during the main deployment
            # We already know which ones failed from the main loop
            if ! grep -q "✅ Successfully deployed: $contract" <<< "$deployment_output" && 
               ! grep -q "✅ Contract already up to date: $contract" <<< "$deployment_output"; then
                echo "❌ $contract - Failed to deploy"
            fi
        done
        echo "----------------------------------------"
        echo -e "\n💡 Some contracts failed to deploy. Check the error messages above for details."
        return 1
    else
        echo -e "\n✅ All contracts processed successfully!"
        return 0
    fi

    }


# Run main function and handle exit code
# set -x  # Debug output disabled (was enabled here)

# Run main function
main "$@"
main_status=$?

# Clean up keosd process
cleanup_keosd $main_status
cleanup_status=$?

# Use the main status unless cleanup failed and main succeeded
if [ $main_status -eq 0 ] && [ $cleanup_status -ne 0 ]; then
    final_status=$cleanup_status
else
    final_status=$main_status
fi

# Print final status
if [ $final_status -eq 0 ]; then
    echo "✅ All deployments completed successfully"
else
    echo "❌ Deployment completed with errors" >&2
fi

exit $final_status

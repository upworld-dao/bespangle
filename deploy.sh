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
NETWORK=""Run # Enable debug output
+ chmod +x ./deploy.sh
+ chmod +x ./docker-run.sh
++ pwd
+ export GITHUB_WORKSPACE=/home/runner/work/bespangle/bespangle
+ GITHUB_WORKSPACE=/home/runner/work/bespangle/bespangle
+ NETWORK=automate-deployment
+ mkdir -p build
+ chmod -R 777 build
+ echo '🚀 Starting deployment to automate-deployment network...'
🚀 Starting deployment to automate-deployment network...
++ pwd
+ echo 'Working directory: /home/runner/work/bespangle/bespangle'
Working directory: /home/runner/work/bespangle/bespangle
+ set -x
++ id -u
++ id -g
++ id -u
++ id -g
+ docker run --rm -e BESPANGLE_IN_DOCKER=true -e GITHUB_WORKSPACE=/github/workspace -e NETWORK=automate-deployment -e DEPLOYER_PRIVATE_KEY=*** -e USER_ID=1001 -e GROUP_ID=118 -v /home/runner/work/bespangle/bespangle:/github/workspace -v /home/runner/work/bespangle/bespangle/build:/github/workspace/build -w /github/workspace --user root bespangle-dev-env bash -c '
    set -ex &&
    echo '\''=== Setting permissions...'\'' &&
    chown -R 1001:118 /github/workspace/build &&
    chmod -R 777 /github/workspace/build &&
    echo '\''=== Starting deployment script...'\'' &&
    ./deploy.sh --network "automate-deployment" --action both
  '
+ echo '=== Setting permissions...'
=== Setting permissions...
+ chown -R 1001:118 /github/workspace/build
+ chmod -R 777 /github/workspace/build
+ echo '=== Starting deployment script...'
=== Starting deployment script...
+ ./deploy.sh --network automate-deployment --action both
+ main --network automate-deployment --action both
+ setup_wallet
Setting up wallet...
+ echo 'Setting up wallet...'
+ mkdir -p /home/developer/eosio-wallet
+ chmod 700 /home/developer/eosio-wallet
+ mkdir -p /home/developer/eosio-wallet/logs
+ chmod 700 /home/developer/eosio-wallet/logs
+ echo 'Starting keosd...'
Starting keosd...
+ KESOD_PID=14
+ echo 'Waiting for keosd to start...'
+ sleep 5
+ keosd --http-server-address=127.0.0.1:8900
Waiting for keosd to start...
+ ps -p 14
Creating new wallet...
+ echo 'Creating new wallet...'
++ cleos wallet create --to-console
+ WALLET_CREATE_OUTPUT='Creating wallet: default
Save password to use in the future to unlock this wallet.
Without password imported keys will not be retrievable.
"PW5HsbTSgAQVGMCu23gbQBgUse2nf3NLLGrCU2bnRerNjF2YrnYq6"'
++ echo 'Creating wallet: default
Save password to use in the future to unlock this wallet.
Without password imported keys will not be retrievable.
"PW5HsbTSgAQVGMCu23gbQBgUse2nf3NLLGrCU2bnRerNjF2YrnYq6"'
++ grep -o 'PW5[^\"]*'
++ head -1
+ WALLET_PASSWORD=PW5HsbTSgAQVGMCu23gbQBgUse2nf3NLLGrCU2bnRerNjF2YrnYq6
+ '[' -z PW5HsbTSgAQVGMCu23gbQBgUse2nf3NLLGrCU2bnRerNjF2YrnYq6 ']'
+ echo PW5HsbTSgAQVGMCu23gbQBgUse2nf3NLLGrCU2bnRerNjF2YrnYq6
+ chmod 600 /home/developer/wallet_password.txt
+ '[' -n *** ']'
+ echo 'Importing private key...'
Importing private key...
+ echo ***
+ cleos wallet import --private-key
private key: imported private key for: EOS89qzdXvKSDU1d5iDt3zBpqEfBb5HUxZM79SUPHZUbSQbGHTH2Z
+ [[ 4 -gt 0 ]]
+ case $1 in
+ NETWORK=automate-deployment
+ shift 2
+ [[ 2 -gt 0 ]]
+ case $1 in
++ echo both
++ tr '[:upper:]' '[:lower:]'
+ ACTION=both
+ shift 2
+ [[ 0 -gt 0 ]]
+ '[' -z automate-deployment ']'
+ [[ ! both =~ ^(build|deploy|both)$ ]]
+ required_commands=('jq' 'cleos')
+ local required_commands
+ for cmd in "${required_commands[@]}"
+ command -v jq
+ for cmd in "${required_commands[@]}"
+ command -v cleos
+ echo -e '\n=== Starting Deployment ==='
=== Starting Deployment ===
+ echo 'Network: automate-deployment'
Network: automate-deployment
Action: both
Loading network configuration...
+ echo 'Action: both'
+ echo -e '\nLoading network configuration...'
+ load_network_config automate-deployment
+ local network=automate-deployment
+ command -v jq
+ '[' '!' -f network_config.json ']'
+ local config_data
++ jq -r --arg net automate-deployment '.networks[$net] // empty' network_config.json
+ config_data='{
  "endpoint": "http://jungle4.cryptolions.io",
  "chain_id": "73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d",
  "system_symbol": "EOS",
  "system_contract": "eosio",
  "accounts": {
    "aemanager": "aemanagerdlt",
    "andemitter": "andemitterdl",
    "authority": "authoritydlt",
    "badgedata": "badgedatadlt",
    "bamanager": "bamanagerdlt",
    "boundedagg": "boundedagdlt",
    "boundedstats": "boundedstdlt",
    "bounties": "bountiesdlta",
    "cumulative": "cumulativdlt",
    "org": "organizatdlt",
    "requests": "requestsdlta",
    "simmanager": "simmanagedlt",
    "simplebadge": "simplebaddlt",
    "statistics": "statisticdlt",
    "subscription": "subscribedlt"
  }
}'
+ '[' -z '{
  "endpoint": "http://jungle4.cryptolions.io",
  "chain_id": "73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d",
  "system_symbol": "EOS",
  "system_contract": "eosio",
  "accounts": {
    "aemanager": "aemanagerdlt",
    "andemitter": "andemitterdl",
    "authority": "authoritydlt",
    "badgedata": "badgedatadlt",
    "bamanager": "bamanagerdlt",
    "boundedagg": "boundedagdlt",
    "boundedstats": "boundedstdlt",
    "bounties": "bountiesdlta",
    "cumulative": "cumulativdlt",
    "org": "organizatdlt",
    "requests": "requestsdlta",
    "simmanager": "simmanagedlt",
    "simplebadge": "simplebaddlt",
    "statistics": "statisticdlt",
    "subscription": "subscribedlt"
  }
}' ']'
++ echo '{
  "endpoint": "http://jungle4.cryptolions.io",
  "chain_id": "73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d",
  "system_symbol": "EOS",
  "system_contract": "eosio",
  "accounts": {
    "aemanager": "aemanagerdlt",
    "andemitter": "andemitterdl",
    "authority": "authoritydlt",
    "badgedata": "badgedatadlt",
    "bamanager": "bamanagerdlt",
    "boundedagg": "boundedagdlt",
    "boundedstats": "boundedstdlt",
    "bounties": "bountiesdlta",
    "cumulative": "cumulativdlt",
    "org": "organizatdlt",
    "requests": "requestsdlta",
    "simmanager": "simmanagedlt",
    "simplebadge": "simplebaddlt",
    "statistics": "statisticdlt",
    "subscription": "subscribedlt"
  }
}'
++ jq -r .endpoint
+ export NETWORK_ENDPOINT=http://jungle4.cryptolions.io
+ NETWORK_ENDPOINT=http://jungle4.cryptolions.io
++ echo '{
  "endpoint": "http://jungle4.cryptolions.io",
  "chain_id": "73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d",
  "system_symbol": "EOS",
  "system_contract": "eosio",
  "accounts": {
    "aemanager": "aemanagerdlt",
    "andemitter": "andemitterdl",
    "authority": "authoritydlt",
    "badgedata": "badgedatadlt",
    "bamanager": "bamanagerdlt",
    "boundedagg": "boundedagdlt",
    "boundedstats": "boundedstdlt",
    "bounties": "bountiesdlta",
    "cumulative": "cumulativdlt",
    "org": "organizatdlt",
    "requests": "requestsdlta",
    "simmanager": "simmanagedlt",
    "simplebadge": "simplebaddlt",
    "statistics": "statisticdlt",
    "subscription": "subscribedlt"
  }
}'
++ jq -r .chain_id
+ export CHAIN_ID=73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d
+ CHAIN_ID=73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d
++ echo '{
  "endpoint": "http://jungle4.cryptolions.io",
  "chain_id": "73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d",
  "system_symbol": "EOS",
  "system_contract": "eosio",
  "accounts": {
    "aemanager": "aemanagerdlt",
    "andemitter": "andemitterdl",
    "authority": "authoritydlt",
    "badgedata": "badgedatadlt",
    "bamanager": "bamanagerdlt",
    "boundedagg": "boundedagdlt",
    "boundedstats": "boundedstdlt",
    "bounties": "bountiesdlta",
    "cumulative": "cumulativdlt",
    "org": "organizatdlt",
    "requests": "requestsdlta",
    "simmanager": "simmanagedlt",
    "simplebadge": "simplebaddlt",
    "statistics": "statisticdlt",
    "subscription": "subscribedlt"
  }
}'
++ jq -r .system_symbol
+ export SYSTEM_SYMBOL=EOS
+ SYSTEM_SYMBOL=EOS
++ echo '{
  "endpoint": "http://jungle4.cryptolions.io",
  "chain_id": "73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d",
  "system_symbol": "EOS",
  "system_contract": "eosio",
  "accounts": {
    "aemanager": "aemanagerdlt",
    "andemitter": "andemitterdl",
    "authority": "authoritydlt",
    "badgedata": "badgedatadlt",
    "bamanager": "bamanagerdlt",
    "boundedagg": "boundedagdlt",
    "boundedstats": "boundedstdlt",
    "bounties": "bountiesdlta",
    "cumulative": "cumulativdlt",
    "org": "organizatdlt",
    "requests": "requestsdlta",
    "simmanager": "simmanagedlt",
    "simplebadge": "simplebaddlt",
    "statistics": "statisticdlt",
    "subscription": "subscribedlt"
  }
}'
++ jq -r .system_contract
+ export SYSTEM_CONTRACT=eosio
+ SYSTEM_CONTRACT=eosio
++ jq -r '.skip_contracts // [] | join(" ")' network_config.json
+ local 'skip_contracts_json=tokenstaker govweight bountmanager'
+ '[' -n 'tokenstaker govweight bountmanager' ']'
+ IFS=' '
+ read -r -a SKIP_CONTRACTS
+ export SKIP_CONTRACTS
++ echo '{
  "endpoint": "http://jungle4.cryptolions.io",
  "chain_id": "73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d",
  "system_symbol": "EOS",
  "system_contract": "eosio",
  "accounts": {
    "aemanager": "aemanagerdlt",
    "andemitter": "andemitterdl",
    "authority": "authoritydlt",
    "badgedata": "badgedatadlt",
    "bamanager": "bamanagerdlt",
    "boundedagg": "boundedagdlt",
    "boundedstats": "boundedstdlt",
    "bounties": "bountiesdlta",
    "cumulative": "cumulativdlt",
    "org": "organizatdlt",
    "requests": "requestsdlta",
    "simmanager": "simmanagedlt",
    "simplebadge": "simplebaddlt",
    "statistics": "statisticdlt",
    "subscription": "subscribedlt"
  }
}'
++ jq -r '.accounts | to_entries[] | "export \(.key | ascii_upcase)=\"\(.value)\""'
+ local 'accounts=export AEMANAGER="aemanagerdlt"
export ANDEMITTER="andemitterdl"
export AUTHORITY="authoritydlt"
export BADGEDATA="badgedatadlt"
export BAMANAGER="bamanagerdlt"
export BOUNDEDAGG="boundedagdlt"
export BOUNDEDSTATS="boundedstdlt"
export BOUNTIES="bountiesdlta"
export CUMULATIVE="cumulativdlt"
export ORG="organizatdlt"
export REQUESTS="requestsdlta"
export SIMMANAGER="simmanagedlt"
export SIMPLEBADGE="simplebaddlt"
export STATISTICS="statisticdlt"
export SUBSCRIPTION="subscribedlt"'
+ '[' -n 'export AEMANAGER="aemanagerdlt"
export ANDEMITTER="andemitterdl"
export AUTHORITY="authoritydlt"
export BADGEDATA="badgedatadlt"
export BAMANAGER="bamanagerdlt"
export BOUNDEDAGG="boundedagdlt"
export BOUNDEDSTATS="boundedstdlt"
export BOUNTIES="bountiesdlta"
export CUMULATIVE="cumulativdlt"
export ORG="organizatdlt"
export REQUESTS="requestsdlta"
export SIMMANAGER="simmanagedlt"
export SIMPLEBADGE="simplebaddlt"
export STATISTICS="statisticdlt"
export SUBSCRIPTION="subscribedlt"' ']'
+ eval 'export AEMANAGER="aemanagerdlt"
export ANDEMITTER="andemitterdl"
export AUTHORITY="authoritydlt"
export BADGEDATA="badgedatadlt"
export BAMANAGER="bamanagerdlt"
export BOUNDEDAGG="boundedagdlt"
export BOUNDEDSTATS="boundedstdlt"
export BOUNTIES="bountiesdlta"
export CUMULATIVE="cumulativdlt"
export ORG="organizatdlt"
export REQUESTS="requestsdlta"
export SIMMANAGER="simmanagedlt"
export SIMPLEBADGE="simplebaddlt"
export STATISTICS="statisticdlt"
export SUBSCRIPTION="subscribedlt"'
++ export AEMANAGER=aemanagerdlt
++ AEMANAGER=aemanagerdlt
++ export ANDEMITTER=andemitterdl
++ ANDEMITTER=andemitterdl
++ export AUTHORITY=authoritydlt
++ AUTHORITY=authoritydlt
++ export BADGEDATA=badgedatadlt
++ BADGEDATA=badgedatadlt
++ export BAMANAGER=bamanagerdlt
++ BAMANAGER=bamanagerdlt
++ export BOUNDEDAGG=boundedagdlt
++ BOUNDEDAGG=boundedagdlt
++ export BOUNDEDSTATS=boundedstdlt
++ BOUNDEDSTATS=boundedstdlt
++ export BOUNTIES=bountiesdlta
++ BOUNTIES=bountiesdlta
++ export CUMULATIVE=cumulativdlt
++ CUMULATIVE=cumulativdlt
++ export ORG=organizatdlt
++ ORG=organizatdlt
++ export REQUESTS=requestsdlta
++ REQUESTS=requestsdlta
++ export SIMMANAGER=simmanagedlt
++ SIMMANAGER=simmanagedlt
++ export SIMPLEBADGE=simplebaddlt
++ SIMPLEBADGE=simplebaddlt
++ export STATISTICS=statisticdlt
++ STATISTICS=statisticdlt
++ export SUBSCRIPTION=subscribedlt
++ SUBSCRIPTION=subscribedlt
+ '[' 0 -eq 1 ']'
+ local current_branch
++ git rev-parse --abbrev-ref HEAD
++ echo unknown
+ current_branch=unknown
+ echo -e '\nDeployment Details:'
+ echo '- Branch: unknown'
+ echo '- Network: automate-deployment'
+ echo '- Endpoint: http://jungle4.cryptolions.io'
+ echo '- Chain ID: 73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d'
+ contracts_to_process=()
+ local contracts_to_process
+ '[' all = all ']'
+ echo -e '\nDiscovering contracts for network: automate-deployment'
+ contracts_to_process=($(jq -r --arg net "$NETWORK" '.networks[$net].accounts | keys[]' "$NETWORK_CONFIG" 2>/dev/null))
Deployment Details:
- Branch: unknown
- Network: automate-deployment
- Endpoint: http://jungle4.cryptolions.io
- Chain ID: 73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d
Discovering contracts for network: automate-deployment
++ jq -r --arg net automate-deployment '.networks[$net].accounts | keys[]' network_config.json
+ '[' 15 -eq 0 ']'
+ '[' 0 -eq 1 ']'
+ '[' '!' -d build ']'
++ id -u
++ id -g
+ chown -R 0:0 build
+ local success_count=0
+ local fail_count=0
+ local skipped_count=0
+ main_status=0
+ cleanup_keosd 0
+ local exit_code=0
+ echo -e '\nCleaning up keosd process...'
+ pkill -f keosd
Cleaning up keosd process...
+ return 0
+ cleanup_status=0
✅ All deployments completed successfully
+ '[' 0 -eq 0 ']'
+ '[' 0 -ne 0 ']'
+ final_status=0
+ '[' 0 -eq 0 ']'
+ echo '✅ All deployments completed successfully'
+ exit 0
+ DEPLOYMENT_EXIT_CODE=0
=== Deployment script exited with code: 0 ===
+ echo '=== Deployment script exited with code: 0 ==='
+ '[' 0 -ne 0 ']'
+ echo '✅ Successfully deployed to automate-deployment network'
✅ Successfully deployed to automate-deployment network

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
                # Using awk for floating point math to avoid bc dependency
                local buffered_ram_output
                buffered_ram_output=$(awk -v ram="$needed_ram" 'BEGIN { printf "%.0f", ram * 1.25 }' 2>&1)
                if [ $? -ne 0 ]; then
                    echo "ERROR: awk failed calculating RAM buffer for $contract. Input RAM: $needed_ram. Error: $buffered_ram_output" >&2
                    # Continue to attempt RAM purchase with unbuffered amount or let buy_ram handle default if needed_ram is now empty
                    # but log the awk failure. The script will likely fail at buy_ram if needed_ram is bad.
                else
                    needed_ram="$buffered_ram_output"
                fi
                echo "📊 Buying RAM with 25% buffer (or original if buffer calc failed): $needed_ram bytes"
            fi
            
            # Try to buy more RAM
            if ! buy_ram "$account" "$account" "$needed_ram"; then
                echo "❌ Failed to buy RAM. Please manually add RAM to the account and try again."
                return 1
            fi
            
            # If we get here, RAM was successfully purchased, so we can retry the deployment
            attempt=$((attempt + 1))
            continue
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
            echo "💡 Insufficient RAM error detected. Consider buying more RAM for the account."
            echo "   Try: cleos -u $NETWORK_ENDPOINT system buyram $account $account \"RAM_AMOUNT\""
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
        
        # If we've reached max attempts, give up
        if [ $attempt -ge $MAX_ATTEMPTS ]; then
            echo "❌ Max retry attempts reached for $account. Giving up."
            return 1
        fi
        
        # Otherwise, wait and retry
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
            fi
        fi
        
        # If action was just 'build', we're done
        if [ "$ACTION" = "build" ]; then
            echo "Build completed. Use 'deploy' action to deploy the contracts."
            return 0
        fi

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
    fi
    
    # Print final status
    echo -e "\n=== Final Status ==="
    echo "✅ Success: $success_count"
    if [ $skipped_count -gt 0 ]; then
        echo "⏩ Skipped: $skipped_count"
    fi
    if [ $fail_count -gt 0 ]; then
        echo "❌ Failed: $fail_count (see details above)" >&2
        echo -e "\n💡 Some contracts failed to deploy. Check the error messages above for details."
        return 1
    elif [ $success_count -eq 0 ] && [ $skipped_count -eq 0 ]; then
        echo "ℹ️  No contracts were processed. Please check your configuration."
        return 1
    else
        echo -e "\n✅ All contracts processed successfully!"
        return 0
    fi

    }


# Run main function and handle exit code
set -x  # Enable debug output

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

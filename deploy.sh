#!/bin/bash

# Automated Antelope Smart Contract Deployment Script
# This script automatically detects the current branch and deploys
# smart contracts to the appropriate network based on branch name

set -e

# Default values
CONFIG_FILE="config.env"
ACTION="both"
CONTRACTS="all"

# Function to display usage information
print_usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -c, --config    Configuration file (default: config.env)"
    echo "  -a, --action    Action to perform: build, deploy, or both (default: both)"
    echo "  -t, --target    Comma-separated list of contracts to target (default: all)"
    echo "  -h, --help      Display this help message"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--config)
            CONFIG_FILE="$2"
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

# Source the configuration file to load contract account variables
# This loads default account names and potentially network-specific overrides (e.g., JUNGLE4_ORG_CONTRACT)
. "$CONFIG_FILE"

# Function to get network-specific or default contract account
# Takes a base variable name (e.g., "ORG_CONTRACT") and checks if a 
# network-specific version exists (e.g., "JUNGLE4_ORG_CONTRACT" if NETWORK is "jungle4").
# Returns the network-specific account if found, otherwise returns the default account.
get_contract_account() {
    local base_var_name=$1
    local network_prefix="$(echo $NETWORK | tr '[:lower:]' '[:upper:]')_"
    local network_var_name="${network_prefix}${base_var_name}"
    
    # Check if the network-specific variable is defined
    if [ -n "${!network_var_name}" ]; then
        echo "${!network_var_name}"
    else
        # Fallback to the default variable
        echo "${!base_var_name}"
    fi
}

# --- Network Validation and Setup (only performed if deploying) ---
# This entire block is skipped if ACTION is set to "build".
NETWORK=""
ENDPOINT=""
CHAIN_ID=""
SYSTEM_SYMBOL=""

if [ "$ACTION" = "deploy" ] || [ "$ACTION" = "both" ]; then
    echo "Deployment action selected, performing network validation..."
    # Check if network_config.json exists before proceeding
    if [ ! -f "network_config.json" ]; then
        echo "Error: network_config.json not found. Required for deployment."
        exit 3
    fi

    # Detect current Git branch
    CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
    if [ $? -ne 0 ]; then
        echo "Warning: Not in a git repository or git command failed."
        # Decide how to handle deployment outside of a known branch - error out or default?
        # For now, defaulting to jungle4 but this might need review.
        echo "Defaulting network to jungle4 for deployment."
        NETWORK="jungle4"
    else
        echo "Current branch: $CURRENT_BRANCH"
        
        # Set network based on branch name
        # Check if branch name is a key in network_config.json
        if command -v jq >/dev/null 2>&1; then
            BRANCH_IS_NETWORK=$(jq --arg branch "$CURRENT_BRANCH" 'has($branch)' network_config.json)
        else
            # Fallback check without jq - less precise, checks if the line starts with "branch":
            if grep -q "^ *\"$CURRENT_BRANCH\":" network_config.json; then
                BRANCH_IS_NETWORK="true"
            else
                BRANCH_IS_NETWORK="false"
            fi
        fi

        if [ "$BRANCH_IS_NETWORK" = "true" ]; then
            NETWORK="$CURRENT_BRANCH"
        else
            echo "Error: Branch '$CURRENT_BRANCH' is not configured for deployment in network_config.json"
            echo "Deployment is only allowed from branches matching configured networks:"
            if command -v jq >/dev/null 2>&1;
                # Use jq to reliably get top-level keys
                then jq -r 'keys_unsorted[]' network_config.json | sed 's/^/- /'
            else 
                # Fallback using awk if jq is not available - extracts keys ending in ": {
                awk -F'"' '/^ *"[a-zA-Z0-9_-]+": *\{/ {print "- " $2}' network_config.json
            fi
            exit 1
        fi
    fi

    echo "Deploying to network: $NETWORK"

    # Parse network configuration from JSON file using jq or fallback
    if command -v jq >/dev/null 2>&1; then
        ENDPOINT=$(jq -r ".$NETWORK.endpoint // empty" network_config.json)
        CHAIN_ID=$(jq -r ".$NETWORK.chain_id // empty" network_config.json)
        SYSTEM_SYMBOL=$(jq -r ".$NETWORK.system_symbol // empty" network_config.json)
    else
        # Fallback to grep/sed if jq is not available (less robust)
        ENDPOINT=$(grep -o "\"$NETWORK\":{[^}]*\"endpoint\":\"[^\"]*\"" network_config.json | sed -E 's/.*"endpoint":"([^"]*).*/\1/')
        CHAIN_ID=$(grep -o "\"$NETWORK\":{[^}]*\"chain_id\":\"[^\"]*\"" network_config.json | sed -E 's/.*"chain_id":"([^"]*).*/\1/')
        SYSTEM_SYMBOL=$(grep -o "\"$NETWORK\":{[^}]*\"system_symbol\":\"[^\"]*\"" network_config.json | sed -E 's/.*"system_symbol":"([^"]*).*/\1/')
    fi

    if [ -z "$ENDPOINT" ] || [ -z "$CHAIN_ID" ] || [ -z "$SYSTEM_SYMBOL" ]; then
        echo "Error: Could not parse required network configuration (endpoint, chain_id, system_symbol) for '$NETWORK' from network_config.json"
        exit 4
    fi

    echo "Network endpoint: $ENDPOINT"
    echo "Chain ID: $CHAIN_ID"
    echo "System Symbol: $SYSTEM_SYMBOL"

    # Create cleos configuration for other scripts to use (only if deploying)
    cat > cleos_config.sh << EOF
#!/bin/bash
# Auto-generated cleos configuration for $NETWORK
cleos_url="$ENDPOINT"
chain_id="$CHAIN_ID"
system_symbol="$SYSTEM_SYMBOL"
EOF
    chmod +x cleos_config.sh
    echo "Generated cleos_config.sh for $NETWORK network."

fi # End of deployment-only network validation & setup


# --- Build & Deploy Configuration ---

# Define paths
# Base path where contract source code directories reside (e.g., ./org/org.cpp)
BASE_CONTRACT_PATH="org"

# Initialize the command part string
D_PART=""

# Read each line from the configuration file to build CMake parameters
while IFS='=' read -r var_name var_value; do
    # Skip empty lines and comments
    if [[ -z "$var_name" || "$var_name" == \#* ]]; then
        continue
    fi
    
    # Trim whitespace and remove potential quotes around the variable value
    var_value=$(echo "$var_value" | tr -d '"' | xargs)
    
    # Append to D_PART if the variable name and value are not empty
    if [ -n "$var_name" ] && [ -n "$var_value" ]; then
        D_PART+="-D${var_name}=${var_value} "
    fi
done < "$CONFIG_FILE"

D_PART=$(echo "$D_PART" | xargs)

echo "CMake parameters: $D_PART"

parse_arguments() {
    # Already set by command line arguments
    # ACTION=$1
    # CONTRACTS=$2
    :
}

# Split CONTRACT into an array based on comma separation
split_contract_names() {
    IFS=',' read -ra ADDR <<< "$CONTRACTS"
    CONTRACT_NAMES=("${ADDR[@]}")
}

# Updated should_process function to support comma-separated contract names
should_process() {
    local contract_name=$1
    if [ "$CONTRACTS" = "all" ]; then
        return 0  # true if "all"
    fi
    for item in "${CONTRACT_NAMES[@]}"; do
        if [ "$item" = "$contract_name" ]; then
            return 0  # true if found
        fi
    done
    return 1  # false if not found
}

# Parse arguments and prepare contract names
parse_arguments 
split_contract_names

# Build a specific contract using eosio-cpp
# Arguments:
#   $1: contract name (e.g., "org") - used for source file path and output wasm/abi names
build_contract() {
    local contract_name=$1
    local contract_base_path=$BASE_CONTRACT_PATH # Should be "org"
    local contract_src_dir="$contract_base_path/src" # Path to the source directory
    local contract_include_dir="$contract_base_path/include" # Path to include dir within org
    local contract_resource_dir="$contract_base_path/resource" # Path to resource dir within org
    local source_file="$contract_src_dir/${contract_name}.cpp"
    local output_dir="build/$contract_name" # Define output directory
    mkdir -p "$output_dir"
    local output_wasm="$output_dir/${contract_name}.wasm"
    local output_abi="$output_dir/${contract_name}.abi"

    if should_process "$contract_name" && { [ "$ACTION" = "build" ] || [ "$ACTION" = "both" ]; }; then
        echo "Building $contract_name from $source_file..."
        
        if [ ! -f "$source_file" ]; then
            echo "Error: Source file not found at '$source_file'"
            return 1 # Indicate error
        fi

        # --- Incremental Build Check --- 
        local rebuild_needed=true # Default to needing rebuild
        if [ -f "$output_wasm" ]; then 
            local output_mtime=$(stat -c %Y "$output_wasm")
            local source_mtime=$(stat -c %Y "$source_file")
            # Find the newest mtime among the source .cpp and all .hpp files in include dir
            local newest_dep_mtime=$(find "$source_file" "$contract_include_dir" -maxdepth 1 -type f \( -name '*.cpp' -o -name '*.hpp' \) -printf '%T@\n' | sort -nr | head -n 1)
            
            # Use bc for floating point comparison potential
            if (( $(echo "$newest_dep_mtime <= $output_mtime" | bc -l) )); then
                echo "Skipping build for $contract_name, output ($output_wasm) is up-to-date."
                rebuild_needed=false
            else
                 echo "Rebuilding $contract_name, source/header newer than output ($output_wasm)."
            fi
        fi

        if [ "$rebuild_needed" = true ] ; then
            # Construct eosio-cpp command
            # Note: Assumes includes are relative to the 'org' directory
            # Outputting WASM/ABI to the build directory.
            echo "Outputting to $output_dir"
            echo "Command: cdt-cpp -abigen -I \"$contract_include_dir\" -R \"$contract_resource_dir\" -contract \"$contract_name\" -o \"$output_wasm\" \"$source_file\""
            cdt-cpp -abigen -I "$contract_include_dir" -R "$contract_resource_dir" -contract "$contract_name" -o "$output_wasm" "$source_file"
            
            # Check if build succeeded by verifying output files exist
        fi # End of rebuild_needed check

        if [ -f "$output_wasm" ] && [ -f "$output_abi" ]; then
            echo "Build of $contract_name complete. Output: $output_wasm, $output_abi"
        else
            echo "Error: Build failed for $contract_name. Check eosio-cpp output above."
            return 1
        fi
    fi
}

# Deploy a specific contract using cleos
# Requires the corresponding .wasm and .abi files to exist (generated by build_contract)
# Arguments:
#   $1: contract file base name (e.g., "org") - used to derive the variable name (ORG_CONTRACT)
#       and locate the .wasm/.abi files (org.wasm, org.abi)
deploy_contract() {
    local contract_file=$1 # e.g., org
    local wasm_file="${contract_file}.wasm"
    local abi_file="${contract_file}.abi"

    if should_process "$contract_file" && { [ "$ACTION" = "deploy" ] || [ "$ACTION" = "both" ]; }; then
        # --- Start Moved Block ---
        local contract_var_name="$(echo ${contract_file} | tr '[:lower:]' '[:upper:]')_CONTRACT" # e.g., ORG_CONTRACT
        local contract_account=$(get_contract_account $contract_var_name)

        if [ -z "$contract_account" ]; then
            echo "Error: Account not defined for $contract_var_name on network $NETWORK."
            return 1 # Indicate error
        fi
        # --- End Moved Block ---

        echo "Deploying $contract_file to account $contract_account on $NETWORK..."
        
        if [ ! -f "$wasm_file" ] || [ ! -f "$abi_file" ]; then
            echo "Error: Missing $wasm_file or $abi_file. Build the contract first."
            return 1
        fi

        echo "Command: cleos -u \"$ENDPOINT\" set contract \"$contract_account\" . \"$wasm_file\" \"$abi_file\" -p \"$contract_account@active\""
        cleos -u "$ENDPOINT" set contract "$contract_account" . "$wasm_file" "$abi_file" -p "$contract_account@active"
        
        echo "Command: cleos -u \"$ENDPOINT\" set account permission \"$contract_account\" active --add-code -p \"$contract_account@active\""
        cleos -u "$ENDPOINT" set account permission "$contract_account" active --add-code -p "$contract_account@active"
        
        echo "Deployment of $contract_file to $contract_account complete."
    fi
}

# --- Contract Build and Deploy Calls ---
# Example structure: build_contract <contract_name>; deploy_contract <contract_file_base_name>
# Ensure the <contract_file_base_name> matches the pattern used to derive the variable name in config.env

# Core contracts
build_contract org; deploy_contract org
build_contract authority; deploy_contract authority
build_contract badgedata; deploy_contract badgedata
build_contract simplebadge; deploy_contract simplebadge
build_contract subscription; deploy_contract subscription

# Manager contracts
build_contract simplemanager; deploy_contract simplemanager # Assuming simplemanager.cpp maps to SIMPLE_MANAGER_CONTRACT
build_contract andemittermanager; deploy_contract andemittermanager
build_contract boundedaggmanager; deploy_contract boundedaggmanager

# Feature contracts
build_contract cumulative; deploy_contract cumulative
build_contract statistics; deploy_contract statistics
build_contract andemitter; deploy_contract andemitter
build_contract boundedagg; deploy_contract boundedagg
build_contract boundedstats; deploy_contract boundedstats
build_contract requests; deploy_contract requests
build_contract bounties; deploy_contract bounties
build_contract govweight; deploy_contract govweight
build_contract tokenstaker; deploy_contract tokenstaker

# Need to determine the correct build/deploy names for these based on source files
# build_contract ???; deploy_contract mutual_recognition_manager 
# build_contract ???; deploy_contract orchestrator_manager
# build_contract ???; deploy_contract giver_rep_manager
# build_contract ???; deploy_contract hll_emitter_manager
# build_contract ???; deploy_contract mutual_recognition
# build_contract ???; deploy_contract hll_emitter
# build_contract ???; deploy_contract giver_rep
# build_contract ???; deploy_contract bounded_hll

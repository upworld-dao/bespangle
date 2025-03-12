#!/bin/sh

CLEOS_URL="http://jungle4.cryptolions.io"

# Check if the configuration file input parameter is provided
if [ -z "$1" ]; then
    echo "Error: No configuration file specified. Usage: $0 <config_file>"
    exit 1
fi

config_file="$1"

# Check if the configuration file exists
if [ ! -f "$config_file" ]; then
    echo "Error: Configuration file not found at '$config_file'"
    exit 2
fi

# Source the configuration file to load variables
. "$config_file"

# Define paths
BASE_CONTRACT_PATH="org"

# Initialize the command part string
D_PART=""

# Read each line from the configuration file
while IFS='=' read -r var_name var_value; do
    # Trim whitespace and remove potential quotes around the variable value
    var_value=$(echo "$var_value" | tr -d '"' | xargs)
    
    # Append to D_PART if the variable name and value are not empty
    if [ -n "$var_name" ] && [ -n "$var_value" ]; then
        D_PART+="-D${var_name}=${var_value} "
    fi
done < "$config_file"

D_PART=$(echo "$D_PART" | xargs)

echo "$D_PART"

_D_PART="-DSIMPLE_MANAGER_CONTRACT=$SIMPLE_MANAGER_CONTRACT -DMUTUAL_RECOGNITION_MANAGER_CONTRACT=$MUTUAL_RECOGNITION_MANAGER_CONTRACT -DORCHESTRATOR_MANAGER_CONTRACT=$ORCHESTRATOR_MANAGER_CONTRACT -DANDEMITTER_MANAGER_CONTRACT=$ANDEMITTER_MANAGER_CONTRACT -DBOUNDED_AGG_MANAGER_CONTRACT=$BOUNDED_AGG_MANAGER_CONTRACT -DGIVER_REP_MANAGER_CONTRACT=$GIVER_REP_MANAGER_CONTRACT -DBOUNDED_HLL_MANAGER_CONTRACT=$BOUNDED_HLL_MANAGER_CONTRACT -DHLL_EMITTER_MANAGER_CONTRACT=$HLL_EMITTER_MANAGER_CONTRACT -DORG_CONTRACT=$ORG_CONTRACT -DAUTHORITY_CONTRACT=$AUTHORITY_CONTRACT -DSIMPLEBADGE_CONTRACT=$SIMPLEBADGE_CONTRACT
-DMUTUAL_RECOGNITION_CONTRACT=$MUTUAL_RECOGNITION_CONTRACT -DCUMULATIVE_CONTRACT=$CUMULATIVE_CONTRACT -DSTATISTICS_CONTRACT=$STATISTICS_CONTRACT -DANDEMITTER_CONTRACT=$ANDEMITTER_CONTRACT -DBOUNDED_AGG_CONTRACT=$BOUNDED_AGG_CONTRACT -DBOUNDED_STATS_CONTRACT=$BOUNDED_STATS_CONTRACT -DHLL_EMITTER_CONTRACT=$HLL_EMITTER_CONTRACT -DGIVER_REP_CONTRACT=$GIVER_REP_CONTRACT -DBOUNDED_HLL_CONTRACT=$BOUNDED_HLL_CONTRACT -DSUBSCRIPTION_CONTRACT=$SUBSCRIPTION_CONTRACT -DREQUESTS_CONTRACT=$REQUESTS_CONTRACT -DBOUNTIES_CONTRACT=$BOUNTIES_CONTRACT -DBADGEDATA_CONTRACT=$BADGEDATA_CONTRACT"
parse_arguments() {
    ACTION=$1
    CONTRACT=$2
}

# Split CONTRACT into an array based on comma separation
split_contract_names() {
    IFS=',' read -ra ADDR <<< "$CONTRACT"
    CONTRACT_NAMES=("${ADDR[@]}")
}

# Updated should_process function to support comma-separated contract names
should_process() {
    local contract_name=$1
    if [ "$CONTRACT" = "all" ]; then
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
parse_arguments "$2" "$3"
split_contract_names

# Modified build_contract function
build_contract() {
    local contract_name=$1
    local contract_path=$BASE_CONTRACT_PATH

    if should_process "$contract_name" && { [ "$ACTION" = "build" ] || [ "$ACTION" = "both" ]; }; then
        eosio-cpp -abigen -I ./include -R ./resource -contract "$contract_name" -o "${contract_name}.wasm" src/"${contract_name}.cpp"
    fi
}

# Modified deploy_contract function
deploy_contract() {
    local contract_name=$1
    local contract_path=$BASE_CONTRACT_PATH
    local contract_file=$2

    if should_process "$contract_file" && { [ "$ACTION" = "deploy" ] || [ "$ACTION" = "both" ]; }; then
        cleos -u "$CLEOS_URL" set contract "$contract_name" . "${contract_file}.wasm" "${contract_file}.abi" -p "$contract_name@active"
        cleos -u "$CLEOS_URL" set account permission "$contract_name" active --add-code -p "$contract_name@active"
    fi
}

# Parse arguments again (if needed)
parse_arguments "$2" "$3"
cd "$BASE_CONTRACT_PATH"
cmake . $_D_PART

build_contract "org"
build_contract "authority"
build_contract "badgedata"
build_contract "cumulative"
build_contract "simplebadge"
build_contract "statistics"
build_contract "simmanager"
build_contract "andemitter"
build_contract "aemanager"
build_contract "subscription"
build_contract "boundedagg"
build_contract "boundedstats"
build_contract "bamanager"
build_contract "requests"
build_contract "bounties"

# simmanager
deploy_contract "$SIMPLE_MANAGER_CONTRACT" "simmanager"

# aemanager
deploy_contract "$ANDEMITTER_MANAGER_CONTRACT" "aemanager"

# bamanager
deploy_contract "$BOUNDED_AGG_MANAGER_CONTRACT" "bamanager"

# org
deploy_contract "$ORG_CONTRACT" "org"

# authority
deploy_contract "$AUTHORITY_CONTRACT" "authority"

# simplebadge
deploy_contract "$SIMPLEBADGE_CONTRACT" "simplebadge"

# metadata
deploy_contract "$BADGEDATA_CONTRACT" "badgedata"

# cumulative
deploy_contract "$CUMULATIVE_CONTRACT" "cumulative"

# statistics
deploy_contract "$STATISTICS_CONTRACT" "statistics"

# andemitter
deploy_contract "$ANDEMITTER_CONTRACT" "andemitter"

# boundedagg
deploy_contract "$BOUNDED_AGG_CONTRACT" "boundedagg"

# boundedstats
deploy_contract "$BOUNDED_STATS_CONTRACT" "boundedstats"

# subscription
deploy_contract "$SUBSCRIPTION_CONTRACT" "subscription"

# requests
deploy_contract "$REQUESTS_CONTRACT" "requests"

# bounties
deploy_contract "$BOUNTIES_CONTRACT" "bounties"


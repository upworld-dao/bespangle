#!/bin/bash

# Usage function to display help
usage() {
    echo "Usage: $0 -r receivers -q quantity"
    echo "receivers should be a comma-separated list."
    exit 1
}

# Initialize variables with default values
receivers=""
quantity=""

# Parse command line options
while getopts ":r:q:" opt; do
  case $opt in
    r) receivers="$OPTARG"
    ;;
    q) quantity="$OPTARG"
    ;;
    \?) echo "Invalid option -$OPTARG" >&2
        usage
    ;;
  esac
done

# Validate required options
if [ -z "$receivers" ] || [ -z "$quantity" ]; then
    usage
fi

# Split comma-separated receivers into an array
IFS=',' read -r -a receiver_array <<< "$receivers"

# Start the JSON string for the actions array
actions_json=""

# Loop through receivers to build the actions JSON
for receiver in "${receiver_array[@]}"; do
    
    # Create the action JSON for the current receiver
    action_json=$(cat <<EOF
    {
      "account": "eosio",
      "name": "buyram",
      "data": {
        "payer": "$receiver",
        "receiver": "$receiver",
        "quant": "${quantity} EOS"
      },
      "authorization": [
        {
          "actor": "$receiver",
          "permission": "active"
        }
      ]
    }
EOF
)

    # Add comma if actions_json is not empty
    [ -n "$actions_json" ] && actions_json+=","
    
    actions_json+="$action_json"
done

# Create the full transaction JSON
TRANSACTION_JSON=$(cat <<EOF
{
  "delay_sec": 0,
  "max_cpu_usage_ms": 0,
  "actions": [ $actions_json ]
}
EOF
)

# Execute the command
cleos -u http://jungle4.cryptolions.io push transaction "$TRANSACTION_JSON"


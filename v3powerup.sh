#!/bin/bash

# Fixed settings
payer="bespangle123"
days=30
net_frac=0.1
cpu_frac=0.1
max_payment="10.0000 EOS"  # Hardcoded maximum payment

# Check if the accounts file input parameter is provided
if [ -z "$1" ]; then
    echo "Error: No accounts file specified. Usage: $0 <accounts_file>"
    exit 1
fi

# Set the accounts file from the input parameter
accounts_file="$1"

# Check if the accounts file exists
if [ ! -f "$accounts_file" ]; then
    echo "Error: File '$accounts_file' not found."
    exit 1
fi

# Source the cleos URL configuration if it exists
if [ -f "./cleos_config.sh" ]; then
    source ./cleos_config.sh
else
    # Default to a known public API node if no configuration is provided
    cleos_url="http://jungle4.cryptolions.io"
fi

# Function to power up accounts by reading each line and extracting the account name
powerup_accounts() {
    while IFS='=' read -r var_name account_name; do
        # Trim whitespace and remove quotes from the account name
        account_name=$(echo "$account_name" | tr -d '" ' | xargs)

        # Check if the line contains an actual account name
        if [ -n "$account_name" ]; then
            echo "Powering up account: $account_name"
            # Uncomment the following line if you wish to use the system powerup command
            # cleos -u $cleos_url system powerup $payer $account_name $days $net_frac $cpu_frac $max_payment

            cleos -u $cleos_url push action eosio powerup "[$payer, $account_name, 1, 200000000000, 200000000000, \"2.0000 EOS\"]" -p $payer

            if [ $? -eq 0 ]; then
                echo "Successfully powered up '$account_name'."
            else
                echo "Failed to power up '$account_name'."
            fi
        fi
    done < "$accounts_file"
}

# Call the function to power up accounts
powerup_accounts

echo "Finished processing all accounts."


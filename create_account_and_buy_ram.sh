#!/bin/bash
# Script to create EOS accounts and buy RAM worth 300 EOS for each account.
# Usage: ./create_accounts_with_ram.sh <accounts_file>
# Each line in the accounts file should be in the format:
# VARIABLE_NAME="accountname"

# Check if the accounts file input parameter is provided.
if [ -z "$1" ]; then
    echo "Error: No accounts file specified. Usage: $0 <accounts_file>"
    exit 1
fi

accounts_file="$1"

# Verify that the accounts file exists.
if [ ! -f "$accounts_file" ]; then
    echo "Error: Accounts file '$accounts_file' not found."
    exit 1
fi

# Set the CLEOS URL for the blockchain node.
CLEOS_URL="http://jungle4.cryptolions.io"

# Set the creator account and default public key for new accounts.
creator="bespangle123"
default_pubkey="EOS8VLRh9JdutT2DFa3zqofQk1h1AYgA9NkjLBuPNpgGHJnxu1XSx"

# Amount of EOS to spend on RAM (300 EOS)
ram_amount="300.0000 EOS"

# Read each line from the accounts file.
while IFS='=' read -r var_name account_name; do
    # Remove quotes and trim whitespace from the account name.
    account_name=$(echo "$account_name" | tr -d '"' | xargs)
    
    # Skip blank lines.
    if [ -z "$account_name" ]; then
        continue
    fi

    echo "Creating account: $account_name"
    
    # Create the account using cleos.
        cleos -u "$CLEOS_URL" system newaccount "$creator" "$account_name" "$default_pubkey" "$default_pubkey" \
        --stake-net "0.1000 EOS" --stake-cpu "0.1000 EOS" --buy-ram-kbytes 4
    if [ $? -eq 0 ]; then
        echo "Successfully created account '$account_name'."
    else
        echo "Failed to create account '$account_name'. Skipping RAM purchase."
        continue
    fi
    
    # Buy RAM for the newly created account.
    echo "Buying RAM for account: $account_name ($ram_amount)"
    cleos -u "$CLEOS_URL" system buyram "$creator" "$account_name" "$ram_amount"
    if [ $? -eq 0 ]; then
        echo "Successfully purchased RAM for '$account_name'."
    else
        echo "Failed to purchase RAM for '$account_name'."
    fi
done < "$accounts_file"

echo "Finished processing all accounts."


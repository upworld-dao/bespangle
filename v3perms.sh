#!/bin/bash
# Check if an accounts file was provided
if [ "$#" -lt 1 ]; then
  echo "Error: No accounts file provided. Usage: $0 <accounts_file>"
  exit 1
fi

ACCOUNTS_FILE="$1"
if [ ! -f "$ACCOUNTS_FILE" ]; then
  echo "Error: Accounts file '$ACCOUNTS_FILE' not found."
  exit 1
fi

# Source the external accounts file to load contract variables
source "$ACCOUNTS_FILE"

# The accounts file should define variables such as:
#   SIMPLE_MANAGER_CONTRACT, SIMPLEBADGE_CONTRACT, BADGEDATA_CONTRACT,
#   ANDEMITTER_CONTRACT, ANDEMITTER_MANAGER_CONTRACT, BOUNDED_AGG_CONTRACT,
#   BOUNDED_AGG_MANAGER_CONTRACT, BOUNDED_STATS_CONTRACT, MUTUAL_RECOGNITION_CONTRACT,
#   MUTUAL_RECOGNITION_MANAGER_CONTRACT, HLL_EMITTER_MANAGER_CONTRACT,
#   HLL_EMITTER_VALIDATION_CONTRACT, GIVER_REP_CONTRACT, GIVER_REP_MANAGER_CONTRACT,
#   BOUNDED_HLL_CONTRACT, BOUNDED_HLL_MANAGER_CONTRACT, SUBSCRIPTION_CONTRACT,
#   CUMULATIVE_CONTRACT, STATISTICS_CONTRACT, AUTHORITY_CONTRACT, etc.
#
# For example, accounts.txt might contain:
#   SIMPLE_MANAGER_CONTRACT="simmanageryy"
#   SIMPLEBADGE_CONTRACT="simplebadgey"
#   BADGEDATA_CONTRACT="orchyyyyyyyy"
#   ANDEMITTER_CONTRACT="andemitteryy"
#   ANDEMITTER_MANAGER_CONTRACT="aemanageryyy"
#   BOUNDED_AGG_CONTRACT="baggyyyyyyyy"
#   BOUNDED_AGG_MANAGER_CONTRACT="bamanageryyy"
#   BOUNDED_STATS_CONTRACT="bstatsyyyyyy"
#   MUTUAL_RECOGNITION_CONTRACT="mrbadgeyyyyy"
#   MUTUAL_RECOGNITION_MANAGER_CONTRACT="mrmanageryyy"
#   HLL_EMITTER_MANAGER_CONTRACT="hllmanageryy"
#   HLL_EMITTER_VALIDATION_CONTRACT="hllemitteryy"
#   GIVER_REP_CONTRACT="giverrepyyyy"
#   GIVER_REP_MANAGER_CONTRACT="grmanageryyy"
#   BOUNDED_HLL_CONTRACT="boundedhllyy"
#   BOUNDED_HLL_MANAGER_CONTRACT="bhllmanagery"
#   SUBSCRIPTION_CONTRACT="subyyyyyyyyy"
#   CUMULATIVE_CONTRACT="cumulativeyy"
#   STATISTICS_CONTRACT="statisticsyy"
#   AUTHORITY_CONTRACT="authorityyyy"
#   (etc.)

# Node URL for cleos
NODE_URL="http://jungle4.cryptolions.io"

# Use AUTHORITY_CONTRACT from the input file as AUTH_ACTOR
AUTH_ACTOR="$AUTHORITY_CONTRACT"
AUTH_PERMISSION="active"

# Helper function that pushes an "addauth" transaction given three parameters:
#  $1 = contract to update
#  $2 = action name
#  $3 = authorized contract (that is being granted permission)
push_auth() {
  local contract="$1"
  local action="$2"
  local auth_contract="$3"

  cleos -u "$NODE_URL" push transaction '{
    "delay_sec": 0,
    "max_cpu_usage_ms": 0,
    "actions": [
      {
        "account": "'"${AUTH_ACTOR}"'",
        "name": "addauth",
        "data": {
          "contract": "'"${contract}"'",
          "action": "'"${action}"'",
          "authorized_contract": "'"${auth_contract}"'"
        },
        "authorization": [
          {
            "actor": "'"${AUTH_ACTOR}"'",
            "permission": "'"${AUTH_PERMISSION}"'"
          }
        ]
      }
    ]
  }'
}

# Define an array of transactions.
# Each entry is defined with three fields:
#   1. The contract to update (sourced from the accounts file)
#   2. The action name (literal)
#   3. The authorized contract (sourced from the accounts file)
#
# In a few cases (e.g. for the HLL emitter) we assume that when it is the
# contract being updated you want the validation version, and when it is being
# granted authority you want the manager version.
transactions=(
  "$SIMPLEBADGE_CONTRACT create $SIMPLE_MANAGER_CONTRACT"
  "$BADGEDATA_CONTRACT initbadge $SIMPLEBADGE_CONTRACT"
  "$BADGEDATA_CONTRACT addfeature $SIMPLE_MANAGER_CONTRACT"
  "$SIMPLEBADGE_CONTRACT issue $SIMPLE_MANAGER_CONTRACT"
  "$BADGEDATA_CONTRACT achievement $SIMPLEBADGE_CONTRACT"
  "$ANDEMITTER_CONTRACT newemission $ANDEMITTER_MANAGER_CONTRACT"
  "$BADGEDATA_CONTRACT addfeature $ANDEMITTER_MANAGER_CONTRACT"
  "$ANDEMITTER_CONTRACT activate $ANDEMITTER_MANAGER_CONTRACT"
  "$SIMPLEBADGE_CONTRACT issue $ANDEMITTER_CONTRACT"
  "$BADGEDATA_CONTRACT addfeature $BOUNDED_AGG_MANAGER_CONTRACT"
  "$BOUNDED_AGG_CONTRACT initagg $BOUNDED_AGG_MANAGER_CONTRACT"
  "$BOUNDED_STATS_CONTRACT activate $BOUNDED_AGG_MANAGER_CONTRACT"
  "$BOUNDED_AGG_CONTRACT initseq $BOUNDED_AGG_MANAGER_CONTRACT"
  "$BOUNDED_AGG_CONTRACT actseq $BOUNDED_AGG_MANAGER_CONTRACT"
  "$MUTUAL_RECOGNITION_CONTRACT create $MUTUAL_RECOGNITION_MANAGER_CONTRACT"
  "$BADGEDATA_CONTRACT initbadge $MUTUAL_RECOGNITION_CONTRACT"
  "$BADGEDATA_CONTRACT addfeature $MUTUAL_RECOGNITION_MANAGER_CONTRACT"
  "$MUTUAL_RECOGNITION_CONTRACT issue $MUTUAL_RECOGNITION_MANAGER_CONTRACT"
  "$BADGEDATA_CONTRACT achievement $MUTUAL_RECOGNITION_CONTRACT"
  "$BADGEDATA_CONTRACT addfeature $HLL_EMITTER_MANAGER_CONTRACT"
  "$SIMPLEBADGE_CONTRACT issue $HLL_EMITTER_VALIDATION_CONTRACT"
  "$BADGEDATA_CONTRACT addfeature $GIVER_REP_MANAGER_CONTRACT"
  "$GIVER_REP_CONTRACT newemission $GIVER_REP_MANAGER_CONTRACT"
  "$GIVER_REP_CONTRACT activate $GIVER_REP_MANAGER_CONTRACT"
  "$SIMPLEBADGE_CONTRACT issue $GIVER_REP_CONTRACT"
  "$BOUNDED_AGG_CONTRACT addinitbadge $BOUNDED_AGG_MANAGER_CONTRACT"
  "$BOUNDED_AGG_CONTRACT endseq $BOUNDED_AGG_MANAGER_CONTRACT"
  "$BADGEDATA_CONTRACT addfeature $BOUNDED_HLL_MANAGER_CONTRACT"
  "$BOUNDED_HLL_CONTRACT newemission $BOUNDED_HLL_MANAGER_CONTRACT"
  "$BOUNDED_HLL_CONTRACT activate $BOUNDED_HLL_MANAGER_CONTRACT"
  "$SIMPLEBADGE_CONTRACT issue $BOUNDED_HLL_CONTRACT"
  "$BOUNDED_AGG_CONTRACT actseqfi $BOUNDED_AGG_MANAGER_CONTRACT"
  "$BOUNDED_AGG_CONTRACT actseqai $BOUNDED_AGG_MANAGER_CONTRACT"
  "$SUBSCRIPTION_CONTRACT billing $SIMPLEBADGE_CONTRACT"
  "$SUBSCRIPTION_CONTRACT billing $CUMULATIVE_CONTRACT"
  "$SUBSCRIPTION_CONTRACT billing $ANDEMITTER_CONTRACT"
  "$SUBSCRIPTION_CONTRACT billing $STATISTICS_CONTRACT"
  "$SUBSCRIPTION_CONTRACT billing $BOUNDED_AGG_CONTRACT"
  "$SUBSCRIPTION_CONTRACT billing $BOUNDED_STATS_CONTRACT"
  "$SUBSCRIPTION_CONTRACT billing $BOUNDED_HLL_CONTRACT"
  "$SUBSCRIPTION_CONTRACT billing $MUTUAL_RECOGNITION_CONTRACT"
  "$BOUNDED_AGG_CONTRACT addbadgeli $BOUNDED_AGG_MANAGER_CONTRACT"
  "$BOUNDED_AGG_CONTRACT pauseall $BOUNDED_AGG_MANAGER_CONTRACT"
  "$BOUNDED_AGG_CONTRACT resumeall $BOUNDED_AGG_MANAGER_CONTRACT"
  "$SIMPLEBADGE_CONTRACT create $BOUNTIES_CONTRACT"
  "$BADGEDATA_CONTRACT addfeature $BOUNTIES_CONTRACT"
  "$ANDEMITTER_CONTRACT nextemission $BOUNTIES_CONTRACT"
  "$ANDEMITTER_CONTRACT newemission $BOUNTIES_CONTRACT"
  "$ANDEMITTER_CONTRACT activate $BOUNTIES_CONTRACT"
  "$ANDEMITTER_CONTRACT deactivate $BOUNTIES_CONTRACT"
  "$ANDEMITTER_CONTRACT deactivate $ANDEMITTER_MANAGER_CONTRACT"
)

# Loop over each transaction, split it into its parts, and push it
for tx in "${transactions[@]}"; do
  # Use default word-splitting on whitespace:
  set -- $tx
  push_auth "$1" "$2" "$3"
done


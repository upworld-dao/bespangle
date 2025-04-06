# Bespangle - Decentralized Badging and Organization Toolkit

Bespangle provides a suite of Antelope smart contracts designed for managing decentralized organizations, issuing badges, handling subscriptions, and more.

## Table of Contents

- [Overview](#overview)
- [Smart Contracts](#smart-contracts)
  - [Core Contracts](#core-contracts)
  - [Manager Contracts](#manager-contracts)
  - [Feature Contracts](#feature-contracts)
- [Prerequisites](#prerequisites)
- [Configuration](#configuration)
  - [`network_config.json`](#network_configjson)
  - [`config.env`](#configenv)
- [Deployment](#deployment)
  - [Branching Strategy](#branching-strategy)
  - [`deploy.sh` Script Usage](#deploysh-script-usage)
  - [GitHub Actions Workflow](#github-actions-workflow)
- [Contract Interactions](#contract-interactions)


## Overview

This repository contains the smart contracts and deployment scripts for the Bespangle toolkit. It allows organizations to manage permissions, issue various types of badges (simple, cumulative, statistical), manage subscriptions, and potentially implement features like bounties and governance weighting on Antelope-based blockchains (EOS, WAX, Telos, etc.).

## Smart Contracts

The system is composed of several interconnected smart contracts:

### 1. Core Infrastructure

These contracts form the foundation for organization management and permissions.

- **`org` (`ORG_CONTRACT`)**: The central registry for organizations. It stores organization details (code, display name, image) and manages which accounts have specific action permissions within that organization's scope across other Bespangle contracts.
  - *Key Actions*: `initorg`, `addactionauth`, `delactionauth`.
  - *Key Tables*: `organizations`, `actionauths`.
- **`authority` (`AUTHORITY_CONTRACT`)**: A global authorization contract used by other contracts to grant *inter-contract* permissions. For example, allowing `simplebadge` to call an action on `badgedata`.
  - *Key Actions*: `addauth`, `delauth`, `checkauth`.
  - *Key Tables*: `auth`.

### 2. Badge Data & Issuance

These contracts handle the definition, issuance, and tracking of different types of badges.

- **`badgedata` (`BADGEDATA_CONTRACT`)**: Stores the definitions and metadata for all badges (symbol, display name, image, description). Also manages badge features (like enabling statistics) and tracks individual badge issuance records (achievements). Acts as the central ledger for who owns which badge.
  - *Key Actions*: `initbadge`, `addfeature`, `issueach`, `destroya`, `addstats`, `removestats`.
  - *Key Tables*: `badges`, `features`, `achievements`, `stats`.
- **`simplebadge` (`SIMPLEBADGE_CONTRACT`)**: The basic badge issuance contract. It takes instructions (e.g., from a manager contract) and executes the actual issuance by calling `issueach` on `badgedata`.
  - *Key Actions*: `initbadge`, `issue`.
  - *Interactions*: Calls `badgedata::issueach`.
- **`cumulative` (`CUMULATIVE_CONTRACT`)**: Listens for badge issuance notifications (`badgedata::issueach`) and maintains a simple, global *cumulative balance* for each badge symbol across all users. Does not track individual issuance events.
  - *Key Actions*: `notifyachieve` (triggered by `badgedata`), `delbalance`, `migratebals`.
  - *Key Tables*: `balance`.
- **`statistics` (`STATISTICS_CONTRACT`)**: Also listens for badge issuance notifications but calculates and stores *organization-specific* statistics and leaderboards for badges. Tracks total issued count, unique holders, and maintains ranked lists.
  - *Key Actions*: `notifyachieve`, `addstatbadge`, `remstatbadge`, `initstats`, `updatestats`, `rebuildstats`.
  - *Key Tables*: `stats`, `leaderboard`.
- **`andemitter` (`ANDEMITTER_CONTRACT`)**: Implements complex badge issuance based on AND logic. Allows defining rules ("emissions") where a target badge is issued only when a user has received a specific set of prerequisite badges.
  - *Key Actions*: `initemission`, `adduser`, `deluser`, `activate`, `deactivate`, `nextemission`.
  - *Key Tables*: `emissions`, `required`, `state`.
- **`boundedagg` (`BOUNDED_AGG_CONTRACT`)**: Creates time-bound or event-bound "sequences" for tracking specific badges. Useful for seasonal leaderboards or badges that only count if earned during a specific campaign. Listens for badge issuance and increments counters within active sequences.
  - *Key Actions*: `initagg`, `initseq`, `actseq`, `endseq`, `notifyachieve`.
  - *Key Tables*: `aggregations`, `sequences`, `usercounts`.
- **`boundedstats` (`BOUNDED_STATS_CONTRACT`)**: Provides statistics specifically for achievements tracked by the `boundedagg` contract. Mirrors the functionality of the `statistics` contract but is scoped by `badge_agg_seq_id`, allowing leaderboards for specific campaigns or seasons defined in `boundedagg`.
  - *Key Actions*: `notifyachieve`, `addstatbadge`, `remstatbadge`, `initseqstats`, `updatesstats`, `rebuildstats`.
  *Key Tables*: `seqstats`, `seqleaderbd`.

### 3. Manager Contracts (Authorization Layer)

These act as intermediaries, providing an authorization layer before relaying actions to the underlying badge issuance or feature contracts. They typically check `org::actionauths` before proceeding.

- **`simmanager` (`SIMPLE_MANAGER_CONTRACT`)**: Manages `simplebadge`. Checks organizational authority before calling `simplebadge::initbadge` or `simplebadge::issue`.
  - *Key Actions*: Mirrors `simplebadge` actions (`initbadge`, `issue`) but adds authorization checks.
  - *Interactions*: Calls `simplebadge`, `authority`, `org`.
- **`aemanager` (`ANDEMITTER_MANAGER_CONTRACT`)**: Manages `andemitter`. Checks organizational authority before relaying calls to `andemitter` actions like `initemission`.
  - *Key Actions*: Mirrors `andemitter` actions, adding authorization checks.
  - *Interactions*: Calls `andemitter`, `authority`, `org`.
- **`bamanager` (`BOUNDED_AGG_MANAGER_CONTRACT`)**: Manages `boundedagg` and `boundedstats`. Checks organizational authority before relaying calls to initialize aggregations/sequences or manage statistics tracking for bounded badges.
  - *Key Actions*: Mirrors `boundedagg` and `boundedstats` management actions, adding authorization checks.
  - *Interactions*: Calls `boundedagg`, `boundedstats`, `authority`, `org`.

### 4. Workflow & Feature Contracts

These contracts enable more complex organizational processes.

- **`requests` (`REQUESTS_CONTRACT`)**: Implements a generic multi-signature approval workflow. Other contracts can initiate a request (e.g., for a badge issuance) that requires approval from a specified list of accounts before being executed.
  - *Key Actions*: `ingestsimple` (starts workflow), `approve`, `reject`, `process` (executes approved requests).
  - *Interactions*: Called by contracts like `bounties`; calls target contracts (e.g., `simplebadge`) upon approval; notifies originator via `sharestatus`.
- **`bounties` (`BOUNTIES_CONTRACT`)**: Manages bounty programs. Allows creating bounties with badge/token rewards, defined reviewers, timelines, and participation rules. Integrates with `requests` for submission review and approval, and with `simplebadge`/`badgedata` for reward issuance.
  - *Key Actions*: `newbounty`, `oldbadge`/`newbadge` (config), `signup`, `submit` (triggers `requests`), `status` (handles `requests` notification).
  - *Interactions*: Calls `requests`, `simplebadge`, `badgedata`. Listens to `requests::sharestatus`. Handles token transfers.
- **`govweight` (`GOV_WEIGHT_CONTRACT`)**: Calculates a governance weight score for users based on configurable factors like cumulative badge balance (`cumulative`), recent badge activity (`boundedagg`), and staked tokens (`tokenstaker`).
  - *Key Actions*: `init`, `update`, `calcweight`.
  - *Interactions*: Reads data from `cumulative`, `boundedagg`, `tokenstaker`, `badgedata`.
- **`tokenstaker` (`TOKEN_STAKER_CONTRACT`)**: Allows users to stake fungible tokens, potentially with a lock-up period. Provides data for the `govweight` contract.
  - *Key Actions*: `init`, `update`, `stake`, `unstake`, `claim`, `on_transfer`.
  - *Interactions*: Provides data to `govweight`. Handles token transfers.

### 5. Subscription Management

- **`subscription` (`SUBSCRIPTION_CONTRACT`)**: Manages subscription packages for organizations. Tracks usage limits (e.g., number of badges issued) and enables/disables packages based on payment or limits. Other contracts call `check_billing` on this contract before performing potentially metered actions.
  - *Key Actions*: `initorg`, `addpkg`, `enablepkg`, `disablepkg`, `checkbilling`, `updateusage`.
  - *Key Tables*: `packages`, `orgsubs`, `usage`.
  - *Interactions*: Called by other contracts to verify subscription status/limits.


## Prerequisites

To build and deploy these contracts, you need:

1.  **Antelope CDT (Contract Development Toolkit):** For compiling C++ contracts to WASM and generating ABIs. (See GitHub Actions workflow for specific version, e.g., v4.1.0).
2.  **`cleos`:** The command-line interface for interacting with Antelope blockchains. (See GitHub Actions workflow for specific version, e.g., Leap v5.0.3).
3.  **`git`:** For checking out the code and managing branches for deployment.
4.  **`jq` (Recommended):** A command-line JSON processor used by the deployment script for robust parsing of `network_config.json`. The script has fallbacks if `jq` is not present, but using `jq` is preferred.

## Configuration

Deployment settings are managed through two main files:

### `network_config.json`

This file maps branch names (which correspond to target networks) to their blockchain details.

```json
{
  "mainnet": {
    "endpoint": "https://eos.greymass.com",
    "chain_id": "aca376f2...",
    "system_symbol": "EOS",
    "system_contract": "eosio"
  },
  "jungle4": {
    "endpoint": "http://jungle4.cryptolions.io",
    "chain_id": "73e4385a...",
    "system_symbol": "EOS",
    "system_contract": "eosio"
  },
  // ... other networks like wax, telos
}
```

- **Key:** The name of the network (e.g., `mainnet`, `jungle4`). This **must** match the Git branch name you intend to deploy from.
- **`endpoint`:** The API endpoint URL for the target blockchain.
- **`chain_id`:** The unique Chain ID of the target blockchain.
- **`system_symbol`:** The core system token symbol (e.g., "EOS", "WAX", "TLOS").
- **`system_contract`:** The name of the core system contract (usually "eosio").

### `config.env`

This file defines the Antelope account names where each contract will be deployed.

```bash
# Default Contract account configuration
ORG_CONTRACT=besporgorgs33
AUTHORITY_CONTRACT=besporgauth33
# ... other default contracts

# --- Jungle4 Testnet Accounts ---
# Network-specific overrides (using NETWORKNAME_ prefix)
JUNGLE4_ORG_CONTRACT="organizatdev"
JUNGLE4_AUTHORITY_CONTRACT="authoritydev"
# ... other jungle4 specific contracts
```

- **Default Accounts:** Variables like `ORG_CONTRACT` define the default account name.
- **Network-Specific Overrides:** You can provide different account names for specific networks by prefixing the variable name with the network name in uppercase, followed by an underscore (e.g., `JUNGLE4_ORG_CONTRACT`). The deployment script automatically picks the network-specific variable if it exists for the target network; otherwise, it falls back to the default.

## Deployment

### Branching Strategy

Deployment is tied to the Git branch name. The `deploy.sh` script and the GitHub Actions workflow expect the current branch name to match a key defined in `network_config.json` (e.g., `mainnet`, `jungle4`, `wax`, `telos`). Deployments from other branches will be rejected unless the script is run with `-a build` only.

### `deploy.sh` Script Usage

The `./deploy.sh` script handles building and deploying the contracts.

**Usage:**

```bash
./deploy.sh [options]
```

**Options:**

- `-c, --config <file>`: Specify a configuration file (default: `config.env`).
- `-a, --action <action>`: Action to perform:
    - `build`: Compile contracts to WASM/ABI only. Network validation is skipped.
    - `deploy`: Deploy pre-built WASM/ABI files. Requires network validation based on the current branch.
    - `both` (default): Perform both `build` and `deploy`. Requires network validation.
- `-t, --target <contracts>`: Comma-separated list of contract base names to target (e.g., `org,authority`). Default is `all`. This affects both build and deploy steps.
- `-h, --help`: Display help message.

**Process:**

1.  **Parse Arguments:** Reads command-line options.
2.  **Load Config:** Sources `config.env` to load account names.
3.  **Network Validation (if deploying):**
    - Checks if the action involves deployment (`deploy` or `both`).
    - Detects the current Git branch.
    - Verifies if the branch name exists as a key in `network_config.json`.
    - If valid, parses the corresponding endpoint, chain ID, and symbol.
    - Generates `cleos_config.sh` with network details for potential use by other scripts (though `deploy.sh` uses the variables directly).
4.  **Build Contracts (if building):**
    - Iterates through the target contracts.
    - Runs `eosio-cpp` for each contract specified in the script's build/deploy call list (near the end of the script) if it matches the `-t` target or if targeting `all`.
5.  **Deploy Contracts (if deploying):**
    - Iterates through the target contracts.
    - Determines the correct deployment account using `get_contract_account` (checks for `NETWORK_CONTRACT_VARIABLE` then falls back to `CONTRACT_VARIABLE`).
    - Runs `cleos set contract` and `cleos set account permission ... --add-code` for each contract specified in the script's build/deploy call list if it matches the `-t` target or if targeting `all`.

**Example:**

```bash
# Build all contracts locally (no deployment, no network check)
./deploy.sh -a build

# Build and deploy only the 'org' and 'authority' contracts from the 'jungle4' branch
# (Assumes you are currently on the 'jungle4' branch)
./deploy.sh -t org,authority

# Deploy all contracts from the 'mainnet' branch (assumes build already done)
# (Assumes you are currently on the 'mainnet' branch)
./deploy.sh -a deploy
```

### GitHub Actions Workflow

The `.github/workflows/deploy.yml` file defines an automated deployment process triggered on pushes to specific branches (`mainnet`, `jungle4`, `wax`, `telos`).

**Workflow:**

1.  **Checkout Code:** Gets the repository code.
2.  **Setup Dependencies:** Installs the required Antelope CDT and `cleos` (Leap) versions.
3.  **Configure Wallet:** Creates a `cleos` wallet and imports the `DEPLOYER_PRIVATE_KEY` (stored as a GitHub secret).
4.  **Run Deployment Script:** Executes `./deploy.sh` (using the default `both` action). The script automatically detects the branch being pushed to and deploys to the corresponding network defined in `network_config.json`.

## Contract Interactions

Here's a breakdown of the key call patterns and notification flows:

**1. Authorization & Permissions:**

*   **User -> Manager Contracts** (`simmanager`, `aemanager`, `bamanager`):
    *   Users initiate actions (e.g., issue badge, configure rules) via Manager contracts.
*   **Manager Contracts -> `org`**: 
    *   Managers call `org` to check `actionauths` table (intra-organization permission check).
*   **Manager Contracts -> Underlying Feature Contracts** (`simplebadge`, `andemitter`, `boundedagg`, `boundedstats`):
    *   If authorized by `org`, the Manager relays the action call to the specific Feature contract.
*   **Setup Actions / Managers -> `authority`**: 
    *   Use `authority::addauth` to grant permissions for one contract to call another (inter-contract permission).
*   **Feature Contracts -> `authority`**: 
    *   May call `authority::checkauth` before performing a critical cross-contract action.

**2. Badge Issuance Flow:**

*   **Defining Badges**: `badgedata` (`badges` table) is the central definition store.
*   **Simple Issuance**: 
    *   `User -> simmanager -> org` (auth check) `-> simplebadge::issue -> badgedata::issueach`
*   **Complex Issuance (AND Emitter)**:
    *   `User -> aemanager -> org` (auth check) `-> andemitter` (setup/logic)
    *   `andemitter` (logic trigger) `-> simplebadge::issue -> badgedata::issueach`
*   **Bounty/Request Issuance**: 
    *   `User -> bounties::submit -> requests::ingestsimple` (start review)
    *   `Reviewer -> requests::approve`
    *   `Off-chain Trigger / Scheduler -> requests::process -> simplebadge::issue -> badgedata::issueach`
*   **Issuance Notifications (`badgedata::issueach`)**: 
    *   `-> cumulative` (updates global count)
    *   `-> statistics` (updates org stats/leaderboards)
    *   `-> boundedagg` (updates counts in active sequences)
    *   `-> boundedstats` (updates stats/leaderboards for active sequences)

**3. Workflow Contracts:**

*   **`bounties -> requests`**: Calls `ingestsimple` to start submission review.
*   **`requests -> bounties`**: Notifies via `sharestatus` on approval/rejection.
*   **`requests -> simplebadge`**: Calls `issue` to grant badge reward upon approved request processing.

**4. Governance & Staking:**

*   **`govweight <- cumulative, boundedagg, tokenstaker, badgedata`**: Reads balances/state from these contracts to calculate weight.
*   **`tokenstaker <-> Token Contract`**: Handles token transfers for staking/claiming.
*   **`tokenstaker -> govweight`**: Provides staked balance info when queried.

**5. Subscription/Billing:**

*   **Most Contracts -> `subscription`**: 
    *   Call `subscription::check_billing` before executing potentially metered actions.

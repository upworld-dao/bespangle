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

- `-n, --network NETWORK`: Target network (e.g., `jungle4`, `mainnet`, `wax`, `telos`). Must match a key in `network_config.json`.
- `-c, --contracts CONTRACTS`: Comma-separated list of contracts to process (default: `all`).
- `-a, --action ACTION`: Action to perform: `build`, `deploy`, or `both` (default: `both`).
- `-v, --verbose`: Enable verbose output for debugging.
- `-h, --help`: Show help message.

**Examples:**

```bash
# Build all contracts locally (no deployment)
./deploy.sh -n jungle4 -a build

# Deploy specific contracts to jungle4
./deploy.sh -n jungle4 -c org,authority -a deploy

# Build and deploy all contracts to mainnet
./deploy.sh -n mainnet -a both

# Deploy with verbose output
./deploy.sh -n wax -a both -v
```

### GitHub Actions Workflow

The `.github/workflows/deploy.yml` file defines an automated deployment process that runs on pushes to specific branches or can be triggered manually.

**Triggering Deployments:**

1. **Automatic Deployment**: Push to any of these branches:
   - `jungle4`
   - `mainnet`
   - `wax`
   - `telos`

2. **Manual Deployment**: Go to GitHub Actions → Workflows → Deploy Antelope Contracts → Run workflow
   - Select the target network
   - Click "Run workflow"

**Workflow Steps:**

1. **Checkout Code**: Gets the repository code
2. **Setup Dependencies**:
   - Installs Antelope CDT (v4.1.0)
   - Installs Leap (v5.0.3)
3. **Configure Wallet**:
   - Creates and unlocks a wallet
   - Imports the `DEPLOYER_PRIVATE_KEY` from GitHub Secrets
4. **Determine Target Network**:
   - For push events: Uses the branch name as the network
   - For manual triggers: Uses the selected network
5. **Verify Network Configuration**:
   - Validates the network exists in `network_config.json`
   - Checks contract accounts are configured
6. **Deploy Contracts**:
   - Builds contracts if needed
   - Deploys to the target network
   - Provides detailed success/failure reporting

**Required Secrets:**

- `DEPLOYER_PRIVATE_KEY`: Private key with permission to deploy to the contract accounts

### Best Practices

1. **Branch Naming**: Always create feature branches from the target network branch
2. **Pull Requests**: Open PRs to the target network branch for review
3. **Testing**: Test deployments on `jungle4` before deploying to production networks
4. **Secrets**: Never commit private keys to the repository
5. **Verification**: After deployment, verify the contract on the blockchain explorer

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

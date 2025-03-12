#include "../include/tokenstaker.hpp"

void tokenstaker::check_auth(name org) {
    // Check if the action is authorized by the org
    require_auth(get_self());
    
    // Check if the contract is authorized to act on behalf of the org
    authority_contract::interface authority(AUTHORITY_CONTRACT, get_self().value);
    authority.check_allow_or_fail(org, get_self());
}

ACTION tokenstaker::init(
    name org,
    symbol token_symbol,
    uint32_t lock_period_sec,
    double governance_weight_ratio
) {
    check_auth(org);
    
    // Validate input parameters
    check(token_symbol.is_valid(), "Invalid token_symbol");
    check(lock_period_sec > 0, "Lock period must be greater than 0");
    check(governance_weight_ratio > 0, "Governance weight ratio must be greater than 0");
    
    // Check if parameters already exist for this org
    stakeparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    check(org_index == params_table.end(), "Staking parameters already initialized for this org");
    
    // Store the parameters
    params_table.emplace(get_self(), [&](auto& p) {
        p.org = org;
        p.token_symbol = token_symbol;
        p.lock_period_sec = lock_period_sec;
        p.governance_weight_ratio = governance_weight_ratio;
        p.last_updated = current_time_point();
    });

    // Initialize the next IDs singleton if it doesn't exist
    nextids_singleton next_ids(get_self(), get_self().value);
    if (!next_ids.exists()) {
        next_ids.set({1, 1}, get_self());
    }

    // Notify org of action
    require_recipient(org);
}

ACTION tokenstaker::update(
    name org,
    symbol token_symbol,
    uint32_t lock_period_sec,
    double governance_weight_ratio
) {
    check_auth(org);
    
    // Validate input parameters
    check(token_symbol.is_valid(), "Invalid token_symbol");
    check(lock_period_sec > 0, "Lock period must be greater than 0");
    check(governance_weight_ratio > 0, "Governance weight ratio must be greater than 0");
    
    // Find existing parameters for this org
    stakeparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    check(org_index != params_table.end(), "Staking parameters not initialized for this org");
    
    // Update the parameters
    params_table.modify(org_index, get_self(), [&](auto& p) {
        p.token_symbol = token_symbol;
        p.lock_period_sec = lock_period_sec;
        p.governance_weight_ratio = governance_weight_ratio;
        p.last_updated = current_time_point();
    });

    // Notify org of action
    require_recipient(org);
}

ACTION tokenstaker::stake(
    name org,
    name account,
    asset quantity,
    string memo
) {
    // Either the account or the contract can call this
    if (has_auth(account)) {
        require_auth(account);
    } else {
        require_auth(get_self());
    }
    
    // Validate input parameters
    check(quantity.amount > 0, "Quantity must be positive");
    
    // Find staking parameters for this org
    stakeparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    check(org_index != params_table.end(), "Staking parameters not initialized for this org");
    
    // Check that the token symbol matches
    check(quantity.symbol == org_index->token_symbol, "Token symbol does not match staking parameters");
    
    // Add the stake
    add_stake(org, account, quantity);
    
    // Notify relevant accounts
    require_recipient(org);
    require_recipient(account);
}

ACTION tokenstaker::unstake(
    name org,
    name account,
    asset quantity,
    string memo
) {
    // Only the account can unstake
    require_auth(account);
    
    // Validate input parameters
    check(quantity.amount > 0, "Quantity must be positive");
    
    // Find staking parameters for this org
    stakeparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    check(org_index != params_table.end(), "Staking parameters not initialized for this org");
    
    // Check that the token symbol matches
    check(quantity.symbol == org_index->token_symbol, "Token symbol does not match staking parameters");
    
    // Process the unstake
    process_unstake(org, account, quantity);
    
    // Notify relevant accounts
    require_recipient(org);
    require_recipient(account);
}

ACTION tokenstaker::claim(
    name org,
    name account
) {
    // Only the account can claim
    require_auth(account);
    
    // Find staking parameters for this org
    stakeparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    check(org_index != params_table.end(), "Staking parameters not initialized for this org");
    
    // Find unstake entries for this account
    unstakeentries_table unstake_table(get_self(), org.value);
    auto by_account_idx = unstake_table.get_index<"byaccount"_n>();
    auto unstake_it = by_account_idx.lower_bound(account.value);
    
    // Check if there are any unstake entries for this account
    check(unstake_it != by_account_idx.end() && unstake_it->account == account, 
          "No unstaked tokens found for this account");
    
    // Current time
    time_point_sec now = current_time_point();
    
    // Process all claimable unstake entries
    bool claimed_any = false;
    while (unstake_it != by_account_idx.end() && unstake_it->account == account) {
        // Check if the unstake entry is claimable
        if (now >= unstake_it->claimable_at) {
            // Send the tokens back to the account
            send_tokens(account, unstake_it->unstaked_amount, "Claimed unstaked tokens");
            
            // Remove the unstake entry
            auto temp_it = unstake_it;
            unstake_it++;
            by_account_idx.erase(temp_it);
            
            claimed_any = true;
        } else {
            unstake_it++;
        }
    }
    
    // Check if any tokens were claimed
    check(claimed_any, "No claimable unstaked tokens found");
    
    // Notify relevant accounts
    require_recipient(org);
    require_recipient(account);
}

void tokenstaker::on_transfer(name from, name to, asset quantity, string memo) {
    // Skip if this is an outgoing transfer or if it's a self-transfer
    if (from == get_self() || to != get_self() || from == to) {
        return;
    }
    
    // Parse the memo to get the org
    // Memo format: "stake:orgname"
    check(memo.substr(0, 6) == "stake:", "Invalid memo format. Use 'stake:orgname'");
    string org_str = memo.substr(6);
    name org = name(org_str);
    
    // Find staking parameters for this org
    stakeparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    check(org_index != params_table.end(), "Staking parameters not initialized for this org");
    
    // Check that the token symbol matches
    check(quantity.symbol == org_index->token_symbol, "Token symbol does not match staking parameters");
    
    // Add the stake
    add_stake(org, from, quantity);
}

void tokenstaker::add_stake(name org, name account, asset quantity) {
    // Get the next stake ID
    uint64_t stake_id = get_next_stake_id();
    
    // Add a new stake entry
    stakeentries_table stake_table(get_self(), org.value);
    stake_table.emplace(get_self(), [&](auto& s) {
        s.id = stake_id;
        s.account = account;
        s.staked_amount = quantity;
        s.staked_at = current_time_point();
    });
}

void tokenstaker::process_unstake(name org, name account, asset quantity) {
    // Find staking parameters for this org
    stakeparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    
    // Get the total staked amount for this account
    asset total_staked = get_staked_balance(org, account, org_index->token_symbol);
    
    // Check if the account has enough staked
    check(total_staked.amount >= quantity.amount, "Not enough tokens staked");
    
    // Find stake entries for this account
    stakeentries_table stake_table(get_self(), org.value);
    auto by_account_idx = stake_table.get_index<"byaccount"_n>();
    auto stake_it = by_account_idx.lower_bound(account.value);
    
    // Unstake the requested amount
    asset remaining_to_unstake = quantity;
    while (stake_it != by_account_idx.end() && stake_it->account == account && remaining_to_unstake.amount > 0) {
        if (stake_it->staked_amount.amount <= remaining_to_unstake.amount) {
            // Unstake the entire entry
            remaining_to_unstake -= stake_it->staked_amount;
            
            // Remove the stake entry
            auto temp_it = stake_it;
            stake_it++;
            by_account_idx.erase(temp_it);
        } else {
            // Partially unstake this entry
            by_account_idx.modify(stake_it, get_self(), [&](auto& s) {
                s.staked_amount -= remaining_to_unstake;
            });
            remaining_to_unstake.amount = 0;
        }
    }
    
    // Get the next unstake ID
    uint64_t unstake_id = get_next_unstake_id();
    
    // Calculate when the tokens will be claimable
    time_point_sec now = current_time_point();
    time_point_sec claimable_at(now.sec_since_epoch() + org_index->lock_period_sec);
    
    // Add a new unstake entry
    unstakeentries_table unstake_table(get_self(), org.value);
    unstake_table.emplace(get_self(), [&](auto& u) {
        u.id = unstake_id;
        u.account = account;
        u.unstaked_amount = quantity;
        u.unstaked_at = now;
        u.claimable_at = claimable_at;
    });
}

void tokenstaker::send_tokens(name to, asset quantity, string memo) {
    // Get the token contract from the symbol
    name token_contract = name("eosio.token"); // Default, should be configured properly
    
    // Send the tokens
    action(
        permission_level{get_self(), "active"_n},
        token_contract,
        "transfer"_n,
        std::make_tuple(get_self(), to, quantity, memo)
    ).send();
}

uint64_t tokenstaker::get_next_stake_id() {
    nextids_singleton next_ids(get_self(), get_self().value);
    check(next_ids.exists(), "Next IDs singleton does not exist");
    
    auto ids = next_ids.get();
    uint64_t next_id = ids.stake_id++;
    next_ids.set(ids, get_self());
    
    return next_id;
}

uint64_t tokenstaker::get_next_unstake_id() {
    nextids_singleton next_ids(get_self(), get_self().value);
    check(next_ids.exists(), "Next IDs singleton does not exist");
    
    auto ids = next_ids.get();
    uint64_t next_id = ids.unstake_id++;
    next_ids.set(ids, get_self());
    
    return next_id;
}

asset tokenstaker::get_staked_balance(name org, name account, symbol token_symbol) {
    // Initialize balance with 0
    asset balance(0, token_symbol);
    
    // Find stake entries for this account
    stakeentries_table stake_table(get_self(), org.value);
    auto by_account_idx = stake_table.get_index<"byaccount"_n>();
    auto stake_it = by_account_idx.lower_bound(account.value);
    
    // Sum up all stake entries for this account
    while (stake_it != by_account_idx.end() && stake_it->account == account) {
        if (stake_it->staked_amount.symbol == token_symbol) {
            balance += stake_it->staked_amount;
        }
        stake_it++;
    }
    
    return balance;
}

double tokenstaker::get_governance_weight(name org, name account, symbol token_symbol) {
    // Find staking parameters for this org
    stakeparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    if (org_index == params_table.end() || org_index->token_symbol != token_symbol) {
        return 0.0;
    }
    
    // Get the staked balance
    asset staked_balance = get_staked_balance(org, account, token_symbol);
    
    // Calculate the governance weight
    double token_amount = static_cast<double>(staked_balance.amount) / 10000.0; // Assuming 4 decimal places
    return token_amount * org_index->governance_weight_ratio;
}

// Register actions with EOSIO dispatcher
EOSIO_DISPATCH(tokenstaker, (init)(update)(stake)(unstake)(claim)(on_transfer)) 
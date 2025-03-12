#include "../include/govweight.hpp"

void govweight::check_auth(name org) {
    // Check if the action is authorized by the org
    require_auth(get_self());
    
    // Check if the contract is authorized to act on behalf of the org
    authority_contract::interface authority(AUTHORITY_CONTRACT, get_self().value);
    authority.check_allow_or_fail(org, get_self());
}

ACTION govweight::init(
    name org,
    symbol gov_badge_symbol,
    uint8_t cumulative_weight,
    uint8_t bounded_weight,
    uint8_t staked_token_weight,
    uint16_t seasons_count,
    uint64_t min_balance,
    symbol token_symbol
) {
    check_auth(org);
    
    // Validate input parameters
    check(gov_badge_symbol.is_valid(), "Invalid gov_badge_symbol");
    check(token_symbol.is_valid(), "Invalid token_symbol");
    check(cumulative_weight + bounded_weight + staked_token_weight == 100, "Weights must sum to 100%");
    check(seasons_count > 0, "Seasons count must be greater than 0");
    
    // Check if parameters already exist for this org
    govparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    check(org_index == params_table.end(), "Governance parameters already initialized for this org");
    
    // Store the parameters
    params_table.emplace(get_self(), [&](auto& p) {
        p.org = org;
        p.gov_badge_symbol = gov_badge_symbol;
        p.cumulative_weight = cumulative_weight;
        p.bounded_weight = bounded_weight;
        p.staked_token_weight = staked_token_weight;
        p.seasons_count = seasons_count;
        p.min_balance = min_balance;
        p.token_symbol = token_symbol;
        p.last_updated = current_time_point();
    });

    // Notify org of action
    require_recipient(org);
}

ACTION govweight::update(
    name org,
    symbol gov_badge_symbol,
    uint8_t cumulative_weight,
    uint8_t bounded_weight,
    uint8_t staked_token_weight,
    uint16_t seasons_count,
    uint64_t min_balance,
    symbol token_symbol
) {
    check_auth(org);
    
    // Validate input parameters
    check(gov_badge_symbol.is_valid(), "Invalid gov_badge_symbol");
    check(token_symbol.is_valid(), "Invalid token_symbol");
    check(cumulative_weight + bounded_weight + staked_token_weight == 100, "Weights must sum to 100%");
    check(seasons_count > 0, "Seasons count must be greater than 0");
    
    // Find existing parameters for this org
    govparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    check(org_index != params_table.end(), "Governance parameters not initialized for this org");
    
    // Update the parameters
    params_table.modify(org_index, get_self(), [&](auto& p) {
        p.gov_badge_symbol = gov_badge_symbol;
        p.cumulative_weight = cumulative_weight;
        p.bounded_weight = bounded_weight;
        p.staked_token_weight = staked_token_weight;
        p.seasons_count = seasons_count;
        p.min_balance = min_balance;
        p.token_symbol = token_symbol;
        p.last_updated = current_time_point();
    });

    // Clear weight cache since parameters changed
    weightcache_table cache_table(get_self(), org.value);
    auto it = cache_table.begin();
    while (it != cache_table.end()) {
        it = cache_table.erase(it);
    }

    // Notify org of action
    require_recipient(org);
}

ACTION govweight::calcweight(name org, name account) {
    // Anyone can call this function to calculate weights
    // Find parameters for this org
    govparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    check(org_index != params_table.end(), "Governance parameters not initialized for this org");
    
    // Calculate the weight
    double weight = calculate_weight(org, account, *org_index);
    
    // Store in cache
    weightcache_table cache_table(get_self(), org.value);
    auto cache_it = cache_table.find(account.value);
    
    if (cache_it == cache_table.end()) {
        cache_table.emplace(get_self(), [&](auto& c) {
            c.account = account;
            c.weight = weight;
            c.calculated_at = current_time_point();
        });
    } else {
        cache_table.modify(cache_it, get_self(), [&](auto& c) {
            c.weight = weight;
            c.calculated_at = current_time_point();
        });
    }
    
    // Return the weight (will be in the action trace)
    print("Weight for ", account, ": ", weight);
    
    // Notify relevant accounts
    require_recipient(org);
    require_recipient(account);
}

double govweight::get_weight(name org, name account) {
    // Check cache first
    weightcache_table cache_table(get_self(), org.value);
    auto cache_it = cache_table.find(account.value);
    
    if (cache_it != cache_table.end()) {
        // Cache hit - return cached value if recent enough (less than 1 hour old)
        time_point_sec now = current_time_point();
        if (now.sec_since_epoch() - cache_it->calculated_at.sec_since_epoch() < 3600) {
            return cache_it->weight;
        }
    }
    
    // Calculate fresh weight
    govparams_table params_table(get_self(), get_self().value);
    auto org_index = params_table.find(org.value);
    if (org_index == params_table.end()) {
        return 0.0; // Not initialized
    }
    
    return calculate_weight(org, account, *org_index);
}

double govweight::calculate_weight(name org, name account, const govparams& params) {
    // Get balances
    asset cumulative_balance = get_cumulative_balance(org, account, params.gov_badge_symbol);
    asset bounded_agg_balance = get_bounded_agg_balance(org, account, params.gov_badge_symbol, params.seasons_count);
    double staked_token_weight = get_staked_token_weight(org, account, params.token_symbol);
    
    // Check minimum balance requirement
    if (cumulative_balance.amount < params.min_balance) {
        return 0.0;
    }
    
    // Calculate weighted score
    double cumulative_score = (static_cast<double>(params.cumulative_weight) / 100.0) * 
                              (static_cast<double>(cumulative_balance.amount) / 10000.0);
    
    double bounded_score = (static_cast<double>(params.bounded_weight) / 100.0) * 
                           (static_cast<double>(bounded_agg_balance.amount) / 10000.0);
    
    double staked_score = (static_cast<double>(params.staked_token_weight) / 100.0) * 
                          staked_token_weight;
    
    // Return the combined weight
    return cumulative_score + bounded_score + staked_score;
}

asset govweight::get_cumulative_balance(name org, name account, symbol badge_symbol) {
    // Get the cumulative balance from the cumulative contract
    cumulative_contract::interface cumulative(CUMULATIVE_CONTRACT, get_self().value);
    return cumulative.get_balance(org, account, badge_symbol);
}

asset govweight::get_bounded_agg_balance(name org, name account, symbol badge_symbol, uint16_t seasons_count) {
    // Get the bounded aggregate balance from the bounded_agg contract
    // This implementation assumes the bounded_agg contract has a function to get aggregated balances
    // across multiple seasons
    boundedagg_contract::interface bounded_agg(BOUNDED_AGG_CONTRACT, get_self().value);
    return bounded_agg.get_balance(org, account, badge_symbol, seasons_count);
}

double govweight::get_staked_token_weight(name org, name account, symbol token_symbol) {
    // Get the staked token weight from the token staker contract
    tokenstaker_contract::interface token_staker(TOKEN_STAKER_CONTRACT, get_self().value);
    
    // Get the staked balance
    asset staked_balance = token_staker.get_staked_balance(org, account, token_symbol);
    
    // Get the governance weight from the staked tokens
    tokenstaker_contract::stakeparams_table params_table(TOKEN_STAKER_CONTRACT, TOKEN_STAKER_CONTRACT);
    auto org_index = params_table.find(org.value);
    
    if (org_index == params_table.end() || org_index->token_symbol != token_symbol) {
        return 0.0;
    }
    
    // Calculate the governance weight
    double token_amount = static_cast<double>(staked_balance.amount) / 10000.0; // Assuming 4 decimal places
    return token_amount * org_index->governance_weight_ratio;
}

// Register actions with EOSIO dispatcher
EOSIO_DISPATCH(govweight, (init)(update)(calcweight)) 
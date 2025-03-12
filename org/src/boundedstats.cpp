#include <boundedstats.hpp>


void boundedstats::notifyachiev(name org, asset badge_asset, name from, name to, string memo, vector<name> notify_accounts) {
    string action_name = "settings";
    string failure_identifier = "CONTRACT: boundedstats, ACTION: " + action_name + ", MESSAGE: ";
    badgestatus_table badgestatus(name(BOUNDED_AGG_CONTRACT), org.value); // Adjust scope as necessary
    auto badge_status_index = badgestatus.get_index<"bybadgestat"_n>(); // Correct index name
    auto hashed_active_status = hash_active_status(badge_asset.symbol, "active"_n, "active"_n);
    auto itr = badge_status_index.find(hashed_active_status);
    uint8_t actions_used = 0;
    while(itr != badge_status_index.end() && itr->badge_symbol == badge_asset.symbol && itr->badge_status == "active"_n && itr->seq_status == "active"_n) {
        statssetting_table _statssetting(get_self(), itr->agg_symbol.code().raw());
        auto statssetting_itr = _statssetting.find(itr->badge_symbol.code().raw());
        if(statssetting_itr != _statssetting.end()) {
            uint64_t new_balance = get_new_balance(to, itr->badge_agg_seq_id);
            uint64_t old_balance = new_balance - badge_asset.amount;
            update_rank(org, to, itr->badge_agg_seq_id, old_balance, new_balance);
            update_count(org, to, itr->badge_agg_seq_id, old_balance, new_balance);
            actions_used ++;
        }

        itr++;
    }

    action {
        permission_level{get_self(), name("active")},
        name(SUBSCRIPTION_CONTRACT),
        name("billing"),
        billing_args {
            .org = org,
            .actions_used = actions_used}
    }.send();
}

ACTION boundedstats::activate(name org, symbol agg_symbol, vector<symbol> badge_symbols) {
    string action_name = "activate";
    string failure_identifier = "CONTRACT: boundedstats, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    statssetting_table _statssetting(get_self(), agg_symbol.code().raw());
    for(auto i = 0 ; i < badge_symbols.size(); i++) {
        auto itr = _statssetting.find(badge_symbols[i].code().raw());
        check(itr == _statssetting.end(), failure_identifier + badge_symbols[i].code().to_string() +" record already present in statssetting table");

        _statssetting.emplace(get_self(), [&](auto& entry) {
            entry.badge_symbol = badge_symbols[i];
        }); 
    }
}

ACTION boundedstats::deactivate(name org, symbol agg_symbol, vector<symbol> badge_symbols) {
    string action_name = "deactivate";
    string failure_identifier = "CONTRACT: boundedstats, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    statssetting_table _statssetting(get_self(), agg_symbol.code().raw());
    for(auto i = 0 ; i < badge_symbols.size(); i++) {
        auto itr = _statssetting.find(badge_symbols[i].code().raw());
        check(itr != _statssetting.end(), failure_identifier + badge_symbols[i].code().to_string() +" record not present");

        _statssetting.erase(itr);
    }
}

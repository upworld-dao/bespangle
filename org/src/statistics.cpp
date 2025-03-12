#include <statistics.hpp>


void statistics::notifyachiev(
    name org,
    asset badge_asset, 
    name from, 
    name to, 
    string memo, 
    vector<name> notify_accounts) {

    string action_name = "notifyachiev";
    string failure_identifier = "CONTRACT: statistics, ACTION: " + action_name + ", MESSAGE: ";
    accounts _accounts(name(CUMULATIVE_CONTRACT), to.value);
    auto accounts_itr = _accounts.find(badge_asset.symbol.code().raw());
    
    uint64_t new_balance = (accounts_itr == _accounts.end()) ? 0 : accounts_itr->balance.amount;
    update_rank(org, to, badge_asset.symbol, new_balance - badge_asset.amount, new_balance);
    update_count(org, to, badge_asset.symbol, new_balance - badge_asset.amount, new_balance);
    
    action {
        permission_level{get_self(), name("active")},
        name(SUBSCRIPTION_CONTRACT),
        name("billing"),
        billing_args {
            .org = org,
            .actions_used = 1}
    }.send();
}

ACTION statistics::dummy() {
}


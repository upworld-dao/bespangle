#include <cumulative.hpp>

void cumulative::notifyachiev(name org, asset badge_asset, name from, name to, std::string memo, std::vector<name> notify_accounts) {
    // Access the accounts table scoped by "to"
    accounts to_accounts(get_self(), to.value);

    // Find the account using the amount's symbol code
    auto existing_account = to_accounts.find(badge_asset.symbol.code().raw());

    if (existing_account == to_accounts.end()) {
        // If the account does not exist, create a new one with the specified badge_asset
        to_accounts.emplace(get_self(), [&](auto& acc) {
            acc.balance = badge_asset;
        });
    } else {
        // If the account exists, modify its balance
        to_accounts.modify(existing_account, get_self(), [&](auto& acc) {
            acc.balance += badge_asset; // Assuming you want to add the badge_asset to the existing balance
        });
    }
    
    action {
        permission_level{get_self(), name("active")},
        name(SUBSCRIPTION_CONTRACT),
        name("billing"),
        billing_args {
            .org = org,
            .actions_used = 1}
    }.send();
}

ACTION cumulative::dummy() {
    // created as a workaround for empty abi.
}

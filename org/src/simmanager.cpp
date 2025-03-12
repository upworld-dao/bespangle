#include <simmanager.hpp>


  ACTION simmanager::initsimple (name authorized, 
      symbol badge_symbol,
      string display_name,
      string ipfs_image,
      string description,
      bool lifetime_aggregate,
      bool lifetime_stats,
      string memo) {

    require_auth(authorized);
    
    string action_name = "initsimple";
    string failure_identifier = "CONTRACT: simmanager, ACTION: " + action_name + ", MESSAGE: ";
    name org = get_org_from_internal_symbol(badge_symbol, failure_identifier);
    name badge = get_name_from_internal_symbol(badge_symbol, failure_identifier);

    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_badge_authority(org, name(action_name), badge, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    }   

    notify_checks_contract(org);
 
    action {
      permission_level{get_self(), name("active")},
      name(SIMPLEBADGE_CONTRACT),
      name("create"),
      createsimple_args {
        .org = org,
        .badge_symbol = badge_symbol,
        .display_name = display_name,
        .ipfs_image = ipfs_image,
        .description = description,
        .memo = memo }
    }.send(); 

    vector<name> consumers;
    consumers.push_back(name(SUBSCRIPTION_CONTRACT));
    if(lifetime_aggregate) {
      consumers.push_back(name(CUMULATIVE_CONTRACT));
    }
    if(lifetime_aggregate && lifetime_stats) {
      consumers.push_back(name(STATISTICS_CONTRACT));
    } else if (!lifetime_aggregate && lifetime_stats) {
      check(false, "Enable Lifetime aggregates to capture Lifetime stats");
    }

    for (auto i = 0 ; i < consumers.size(); i++) {
      action {
        permission_level{get_self(), name("active")},
        name(BADGEDATA_CONTRACT),
        name("addfeature"),
        addfeature_args {
          .org = org,
          .badge_symbol = badge_symbol,
          .notify_account = consumers[i],
          .memo = memo}
      }.send();
    }
     
  }

  ACTION simmanager::simplebatch (name authorized,
      symbol badge_symbol,
      uint64_t amount,
      vector<name> to, 
      string memo) {
    require_auth(authorized);
    
    string action_name = "simplebatch";
    string failure_identifier = "CONTRACT: simmanager, ACTION: " + action_name + ", MESSAGE: ";
    name org = get_org_from_internal_symbol(badge_symbol, failure_identifier);
    name badge = get_name_from_internal_symbol(badge_symbol, failure_identifier);
    
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_badge_authority(org, name(action_name), badge, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    }   

    notify_checks_contract(org);
    asset badge_asset(amount, badge_symbol);
    
    for( auto i = 0; i < to.size(); i++ ) {
      action {
      permission_level{get_self(), name("active")},
      name(SIMPLEBADGE_CONTRACT),
      name("issue"),
      issuesimple_args {
        .org = org,
        .badge_asset = badge_asset,
        .to = to[i],
        .memo = memo }
      }.send();
    }
  }


  ACTION simmanager::givesimple (name authorized,
      symbol badge_symbol,
      uint64_t amount, 
      name to, 
      string memo ) {
    require_auth(authorized);
    string action_name = "givesimple";
    string failure_identifier = "CONTRACT: simmanager, ACTION: " + action_name + ", MESSAGE: ";
    name org = get_org_from_internal_symbol(badge_symbol, failure_identifier);
    name badge = get_name_from_internal_symbol(badge_symbol, failure_identifier);

    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_badge_authority(org, name(action_name), badge, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    }  

    notify_checks_contract(org);
    asset badge_asset(amount, badge_symbol);

    action {
    permission_level{get_self(), name("active")},
    name(SIMPLEBADGE_CONTRACT),
    name("issue"),
    issuesimple_args {
      .org = org,
      .badge_asset = badge_asset,
      .to = to,
      .memo = memo }
    }.send();


  }

ACTION simmanager::addbadgeauth(name org, name action, name badge, name authorized_account) {
    require_auth(org);

    badgeauths_table badgeauths(get_self(), org.value);
    auto secondary_index = badgeauths.get_index<"byactionbadge"_n>();
    uint128_t secondary_key = badgeauths::combine_names(action, badge);
    auto itr = secondary_index.find(secondary_key);

    if (itr != secondary_index.end() && itr->action == action && itr->badge == badge) {
        bool is_account_already_authorized = find(itr->authorized_accounts.begin(), itr->authorized_accounts.end(), authorized_account) != itr->authorized_accounts.end();
        check(!is_account_already_authorized, "Account is already authorized for this badge and action.");

        badgeauths.modify(*itr, get_self(), [&](auto& row) {
            row.authorized_accounts.push_back(authorized_account);
        });
    } else {
        badgeauths.emplace(get_self(), [&](auto& row) {
            row.id = badgeauths.available_primary_key();
            row.action = action;
            row.badge = badge;
            row.authorized_accounts.push_back(authorized_account);
        });
    }
}

ACTION simmanager::delbadgeauth(name org, name action, name badge, name authorized_account) {
    require_auth(org);

    string action_name = "delbadgeauth";
    string failure_identifier = "CONTRACT: baval, ACTION: " + action_name + ", MESSAGE: ";
 
    badgeauths_table badgeauths(get_self(), org.value);
    auto secondary_index = badgeauths.get_index<"byactionbadge"_n>();
    uint128_t secondary_key = badgeauths::combine_names(action, badge);
    auto itr = secondary_index.find(secondary_key);

    check(itr != secondary_index.end() && itr->action == action && itr->badge == badge, failure_identifier + "badge with specified action and badge not found");

    auto acc_itr = find(itr->authorized_accounts.begin(), itr->authorized_accounts.end(), authorized_account);
    check(acc_itr != itr->authorized_accounts.end(), failure_identifier + "Account not found in authorized accounts");
    
    bool should_erase = false;
    secondary_index.modify(itr, get_self(), [&](auto& row) {
        row.authorized_accounts.erase(acc_itr);
        should_erase = row.authorized_accounts.empty();
    });

    if (should_erase) {
        secondary_index.erase(itr);
    }
}

ACTION simmanager::addactionauth(name org, name action, name authorized_account) {
    require_auth(org);

    actionauths_table _actionauths(get_self(), org.value);
    auto itr = _actionauths.find(action.value);

    if (itr != _actionauths.end()) {
        _actionauths.modify(itr, get_self(), [&](auto& row) {
            auto it = find(row.authorized_accounts.begin(), row.authorized_accounts.end(), authorized_account);
            if (it == row.authorized_accounts.end()) {
                row.authorized_accounts.push_back(authorized_account);
            } else {
                check(false, "Account is already authorized for this action.");
            }
        });
    } else {
        _actionauths.emplace(get_self(), [&](auto& row) {
            row.action = action;
            row.authorized_accounts.push_back(authorized_account);
        });
    }
}

ACTION simmanager::delactionauth(name org, name action, name authorized_account) {
    require_auth(org);

    actionauths_table _actionauths(get_self(), org.value);
    auto itr = _actionauths.find(action.value);

    if (itr != _actionauths.end()) {
        bool should_erase = false;

        _actionauths.modify(itr, get_self(), [&](auto& row) {
            auto it = find(row.authorized_accounts.begin(), row.authorized_accounts.end(), authorized_account);
            if (it != row.authorized_accounts.end()) {
                row.authorized_accounts.erase(it);
            }
            
            should_erase = row.authorized_accounts.empty();
        });

        if (should_erase) {
            _actionauths.erase(itr);
        }
    }
    check(false, "account already not authorized");
}

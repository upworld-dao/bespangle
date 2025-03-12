#include <aemanager.hpp>

ACTION aemanager::newemission(name authorized, 
  symbol emission_symbol,
  string display_name, 
  string ipfs_description,
  vector<asset> emitter_criteria, 
  vector<asset> emit_badges, 
  bool cyclic) {
  require_auth(authorized);
  string action_name = "newemission";
  string failure_identifier = "CONTRACT: aemanager, ACTION: " + action_name + ", MESSAGE: ";
  name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
  name emission = get_name_from_internal_symbol(emission_symbol, failure_identifier);

  if(org != authorized) {
    bool has_authority = has_action_authority(org, name(action_name), authorized) ||
      has_emission_authority(org, name(action_name), emission, authorized);
    check (has_authority, failure_identifier + "Unauthorized account to execute action");
  }  

  vector<contract_asset> emit_assets;

  for(asset a : emitter_criteria) {
    check(org == get_org_from_internal_symbol(emission_symbol, failure_identifier), "org not same for all assets in emitter_criteria");
  }
  for(asset a : emit_badges) {
    check(org == get_org_from_internal_symbol(emission_symbol, failure_identifier), "org not same for all assets in emit_badges");
  }
  notify_checks_contract(org);
  for(auto i = 0 ; i < emit_badges.size(); i++) {
    emit_assets.push_back (contract_asset {
      .contract = name(SIMPLEBADGE_CONTRACT),
      .emit_asset = emit_badges[i]
    });
  }

  action{
    permission_level{get_self(), name("active")},
    name(ANDEMITTER_CONTRACT),
    name("newemission"),
    init_args{
      .org = org,
      .display_name = display_name,
      .ipfs_description = ipfs_description,
      .emission_symbol = emission_symbol,
      .emitter_criteria = emitter_criteria,
      .emit_assets = emit_assets,
      .cyclic = cyclic
    }
  }.send();

  for (auto i = 0; i < emitter_criteria.size(); i++) {
    action{
      permission_level{get_self(), name("active")},
      name(BADGEDATA_CONTRACT),
      name("addfeature"),
      addfeature_args{
        .org = org,
        .badge_symbol = emitter_criteria[i].symbol,
        .notify_account = name(ANDEMITTER_CONTRACT),
        .memo = ""
      }
    }.send();
  }
}

ACTION aemanager::activate(name authorized, symbol emission_symbol) {
  require_auth(authorized);
  string action_name = "activate";
  string failure_identifier = "CONTRACT: aemanager, ACTION: " + action_name + ", MESSAGE: ";  
  name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
  name emission = get_name_from_internal_symbol(emission_symbol, failure_identifier);

  if(org != authorized) {
    bool has_authority = has_action_authority(org, name(action_name), authorized) ||
      has_emission_authority(org, name(action_name), emission, authorized);
    check (has_authority, failure_identifier + "Unauthorized account to execute action");
  }   
  notify_checks_contract(org);

  action{
    permission_level{get_self(), name("active")},
    name(ANDEMITTER_CONTRACT),
    name("activate"),
    activate_args{
      .org = org,
      .emission_symbol = emission_symbol
    }
  }.send();
}

ACTION aemanager::deactivate(name authorized, symbol emission_symbol) {
  require_auth(authorized);
  string action_name = "deactivate";
  string failure_identifier = "CONTRACT: aemanager, ACTION: " + action_name + ", MESSAGE: ";
  name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
  name emission = get_name_from_internal_symbol(emission_symbol, failure_identifier);


  if(org != authorized) {
    bool has_authority = has_action_authority(org, name(action_name), authorized) ||
      has_emission_authority(org, name(action_name), emission, authorized);
    check (has_authority, failure_identifier + "Unauthorized account to execute action");
  }  
  notify_checks_contract(org);
  action{
    permission_level{get_self(), name("active")},
    name(ANDEMITTER_CONTRACT),
    name("deactivate"),
    deactivate_args{
      .org = org,
      .emission_symbol = emission_symbol
    }
  }.send();
}

ACTION aemanager::addemissauth(name org, name action, name emission, name authorized_account) {
    require_auth(org);

    string action_name = "delemissauth";
    string failure_identifier = "CONTRACT: aemanager, ACTION: " + action_name + ", MESSAGE: ";

    emissauths_table emissauths(get_self(), org.value);
    auto secondary_index = emissauths.get_index<"actionemiss"_n>();
    uint128_t secondary_key = emissauths::combine_names(action, emission);
    auto itr = secondary_index.find(secondary_key);

    if (itr != secondary_index.end() && itr->action == action && itr->emission == emission) {
        bool is_account_already_authorized = find(itr->authorized_accounts.begin(), itr->authorized_accounts.end(), authorized_account) != itr->authorized_accounts.end();
        check(!is_account_already_authorized, "Account is already authorized for this emission and action.");

        emissauths.modify(*itr, get_self(), [&](auto& row) {
            row.authorized_accounts.push_back(authorized_account);
        });
    } else {
        emissauths.emplace(get_self(), [&](auto& row) {
            row.id = emissauths.available_primary_key();
            row.action = action;
            row.emission = emission;
            row.authorized_accounts.push_back(authorized_account);
        });
    }
}

ACTION aemanager::delemissauth(name org, name action, name emission, name authorized_account) {
    require_auth(org);

    string action_name = "delemissauth";
    string failure_identifier = "CONTRACT: aemanager, ACTION: " + action_name + ", MESSAGE: ";
 

    emissauths_table emissauths(get_self(), org.value);
    auto secondary_index = emissauths.get_index<"actionemiss"_n>();
    uint128_t secondary_key = emissauths::combine_names(action, emission);
    auto itr = secondary_index.find(secondary_key);

    check(itr != secondary_index.end() && itr->action == action && itr->emission == emission, failure_identifier + "specified action and emission not found");

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

ACTION aemanager::addactionauth(name org, name action, name authorized_account) {
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

ACTION aemanager::delactionauth(name org, name action, name authorized_account) {
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


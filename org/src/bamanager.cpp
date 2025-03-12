#include <bamanager.hpp>

ACTION bamanager::initagg(name authorized, symbol agg_symbol, vector<symbol> badge_symbols, vector<symbol> stats_badge_symbols, string ipfs_image, string display_name, string description) {
    require_auth(authorized);
    string action_name = "initagg";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    }  
    for(auto i = 0 ; i < badge_symbols.size(); i++) {
      check(org == get_org_from_internal_symbol(badge_symbols[i], failure_identifier), failure_identifier + "Org mismatch for badge " + badge_symbols[i].code().to_string());
      action {
        permission_level{get_self(), name("active")},
        name(BADGEDATA_CONTRACT),
        name("addfeature"),
        addfeature_args{
          .org = org,
          .badge_symbol = badge_symbols[i],
          .notify_account = name(BOUNDED_AGG_CONTRACT),
          .memo = ""
        }
      }.send();
    }

    for(auto i = 0 ; i < stats_badge_symbols.size(); i++) {
      check(org == get_org_from_internal_symbol(stats_badge_symbols[i], failure_identifier), failure_identifier + "Org mismatch for badge " + stats_badge_symbols[i].code().to_string());
      auto it = std::find(badge_symbols.begin(), badge_symbols.end(), stats_badge_symbols[i]);
      check(it != badge_symbols.end(), failure_identifier + "all element in stats_badge_symbols should be present in badge_symbols");
      action {
        permission_level{get_self(), name("active")},
        name(BADGEDATA_CONTRACT),
        name("addfeature"),
        addfeature_args{
          .org = org,
          .badge_symbol = stats_badge_symbols[i],
          .notify_account = name(BOUNDED_STATS_CONTRACT),
          .memo = ""
        }
      }.send();
    }

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("initagg"),
      initagg_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .init_badge_symbols = badge_symbols,
        .ipfs_image = ipfs_image,
        .display_name = display_name,
        .description = description
      }
    }.send();

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_STATS_CONTRACT),
      name("activate"),
      stats_activate_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .badge_symbols = stats_badge_symbols
      }
    }.send();
}

ACTION bamanager::addinitbadge(name authorized, symbol agg_symbol, vector<symbol> badge_symbols) {
    require_auth(authorized);
    string action_name = "addinitbadge";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    for(auto i = 0 ; i < badge_symbols.size(); i++) {
      check(org == get_org_from_internal_symbol(badge_symbols[i], failure_identifier), failure_identifier + "Org mismatch for badge " + badge_symbols[i].code().to_string());
      action {
        permission_level{get_self(), name("active")},
        name(BADGEDATA_CONTRACT),
        name("addfeature"),
        addfeature_args{
          .org = org,
          .badge_symbol = badge_symbols[i],
          .notify_account = name(BOUNDED_AGG_CONTRACT),
          .memo = ""
        }
      }.send();
    }

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("addinitbadge"),
      addinitbadge_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .badge_symbols = badge_symbols
      }
    }.send();

}
ACTION bamanager::reminitbadge(name authorized, symbol agg_symbol, vector<symbol> badge_symbols) {
    require_auth(authorized);
    string action_name = "reminitbadge";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    for(auto i = 0 ; i < badge_symbols.size(); i++) {
      check(org == get_org_from_internal_symbol(badge_symbols[i], failure_identifier), failure_identifier + "Org mismatch for badge " + badge_symbols[i].code().to_string());
      action {
        permission_level{get_self(), name("active")},
        name(BADGEDATA_CONTRACT),
        name("addfeature"),
        addfeature_args{
          .org = org,
          .badge_symbol = badge_symbols[i],
          .notify_account = name(BOUNDED_AGG_CONTRACT),
          .memo = ""
        }
      }.send();
    }

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("addinitbadge"),
      addinitbadge_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .badge_symbols = badge_symbols
      }
    }.send();
}
ACTION bamanager::addstatbadge(name authorized, symbol agg_symbol, vector<symbol> badge_symbols) {
    require_auth(authorized);
    string action_name = "addstatbadge";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    for(auto i = 0 ; i < badge_symbols.size(); i++) {
      check(org == get_org_from_internal_symbol(badge_symbols[i], failure_identifier), failure_identifier + "Org mismatch for badge " + badge_symbols[i].code().to_string());
      action {
        permission_level{get_self(), name("active")},
        name(BADGEDATA_CONTRACT),
        name("addfeature"),
        addfeature_args{
          .org = org,
          .badge_symbol = badge_symbols[i],
          .notify_account = name(BOUNDED_AGG_CONTRACT),
          .memo = ""
        }
      }.send();
      
      action {
        permission_level{get_self(), name("active")},
        name(BADGEDATA_CONTRACT),
        name("addfeature"),
        addfeature_args{
          .org = org,
          .badge_symbol = badge_symbols[i],
          .notify_account = name(BOUNDED_STATS_CONTRACT),
          .memo = ""
        }
      }.send();
    }

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_STATS_CONTRACT),
      name("activate"),
      stats_activate_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .badge_symbols = badge_symbols
      }
    }.send();

}
ACTION bamanager::remstatbadge(name authorized, symbol agg_symbol, vector<symbol> badge_symbols) {
    require_auth(authorized);
    string action_name = "remstatbadge";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    for(auto i = 0 ; i < badge_symbols.size(); i++) {
      check(org == get_org_from_internal_symbol(badge_symbols[i], failure_identifier), failure_identifier + "Org mismatch for badge " + badge_symbols[i].code().to_string());
    }

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_STATS_CONTRACT),
      name("deactivate"),
      stats_deactivate_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .badge_symbols = badge_symbols
      }
    }.send();
}

ACTION bamanager::initseq(name authorized, symbol agg_symbol, vector<symbol> badge_symbols, string sequence_description) {
    require_auth(authorized);
    string action_name = "initseq";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 
    
    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("initseq"),
      initseq_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .sequence_description = sequence_description
      }
    }.send();

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("addbadgeli"),
      addbadgeli_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .badge_symbols = badge_symbols
      }
    }.send();
}

ACTION bamanager::actseq(name authorized, symbol agg_symbol, vector<uint64_t> seq_ids) {
    require_auth(authorized);
    
    string action_name = "actseq";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("actseq"),
      actseq_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .seq_ids = seq_ids
      }
    }.send();
   
}

ACTION bamanager::actseqai(name authorized, symbol agg_symbol) {
    require_auth(authorized);

    string action_name = "actseqai";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 
        
    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("actseqai"),
      actseqai_args {
        .org = org,
        .agg_symbol = agg_symbol
      }
    }.send();

}

ACTION bamanager::actseqfi(name authorized, symbol agg_symbol) {
    require_auth(authorized);

    string action_name = "actseqfi";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);

    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("actseqfi"),
      actseqfi_args {
        .org = org,
        .agg_symbol = agg_symbol
      }
    }.send();

}

ACTION bamanager::endseq(name authorized, symbol agg_symbol, vector<uint64_t> seq_ids) {
    require_auth(authorized);

    string action_name = "endseq";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 
    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("endseq"),
      endseq_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .seq_ids = seq_ids
      }
    }.send();
}

ACTION bamanager::endseqaa(name authorized, symbol agg_symbol) {
    require_auth(authorized);

    string action_name = "endseqaa";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 
    
    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("endseqaa"),
      endseqaa_args {
        .org = org,
        .agg_symbol = agg_symbol
      }
    }.send();
}

ACTION bamanager::endseqfa(name authorized, symbol agg_symbol) {
    require_auth(authorized);

    string action_name = "endseqfa";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);

    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 
    
    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("endseqfa"),
      actseq_args {
        .org = org,
        .agg_symbol = agg_symbol
      }
    }.send();
}

ACTION bamanager::addbadge(name authorized, symbol agg_symbol, vector<uint64_t> seq_ids, vector<symbol> badge_symbols) {
    require_auth(authorized);

    string action_name = "addbadge";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    for(auto i = 0 ; i < badge_symbols.size(); i++) {
      check(org == get_org_from_internal_symbol(badge_symbols[i], failure_identifier), failure_identifier + "Org mismatch for badge " + badge_symbols[i].code().to_string());
      action {
        permission_level{get_self(), name("active")},
        name(BADGEDATA_CONTRACT),
        name("addfeature"),
        addfeature_args{
          .org = org,
          .badge_symbol = badge_symbols[i],
          .notify_account = name(BOUNDED_AGG_CONTRACT),
          .memo = ""
        }
      }.send();
    }

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("addbadge"),
      addbadge_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .seq_ids = seq_ids,
        .badge_symbols = badge_symbols
      }
    }.send();
}

ACTION bamanager::pauseall(name authorized, symbol agg_symbol, uint64_t seq_id) {
    require_auth(authorized);

    string action_name = "pauseall";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("pauseall"),
      pauseall_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .seq_id = seq_id
      }
    }.send();

}
ACTION bamanager::pausebadge(name authorized, symbol agg_symbol, uint64_t badge_agg_seq_id) {
    require_auth(authorized);

    string action_name = "pausebadge";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("pausebadge"),
      pausebadge_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .badge_agg_seq_id = badge_agg_seq_id
      }
    }.send();
}
ACTION bamanager::pausebadges(name authorized, symbol agg_symbol, uint64_t seq_id, vector<symbol> badge_symbols) {
    require_auth(authorized);

    string action_name = "pausebadges";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    for(auto i = 0 ; i < badge_symbols.size(); i++) {
      check(org == get_org_from_internal_symbol(badge_symbols[i], failure_identifier), failure_identifier + "Org mismatch for badge " + badge_symbols[i].code().to_string());
    }

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("pausebadges"),
      pausebadges_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .seq_id = seq_id,
        .badge_symbols = badge_symbols
      }
    }.send();
}
ACTION bamanager::pauseallfa(name authorized, symbol agg_symbol) {
    require_auth(authorized);

    string action_name = "pauseallfa";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("pauseallfa"),
      pauseallfa_args {
        .org = org,
        .agg_symbol = agg_symbol
      }
    }.send();
}
ACTION bamanager::resumeall(name authorized, symbol agg_symbol, uint64_t seq_id) {
    require_auth(authorized);

    string action_name = "resumeall";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";

    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("resumeall"),
      resumeall_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .seq_id = seq_id
      }
    }.send();
}
ACTION bamanager::resumebadge(name authorized, symbol agg_symbol, uint64_t badge_agg_seq_id) {
    require_auth(authorized);

    string action_name = "resumebadge";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("resumebadge"),
      resumebadge_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .badge_agg_seq_id = badge_agg_seq_id
      }
    }.send();
}
ACTION bamanager::resumebadges(name authorized, symbol agg_symbol, uint64_t seq_id, vector<symbol> badge_symbols) {
    require_auth(authorized);

    string action_name = "resumebadges";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
    
    name org = get_org_from_internal_symbol(agg_symbol, failure_identifier);
    name agg = get_name_from_internal_symbol(agg_symbol, failure_identifier);
    notify_checks_contract(org);
    if(org != authorized) {
      bool has_authority = has_action_authority(org, name(action_name), authorized) ||
        has_agg_authority(org, name(action_name), agg, authorized);
      check (has_authority, failure_identifier + "Unauthorized account to execute action");
    } 

    for(auto i = 0 ; i < badge_symbols.size(); i++) {
      check(org == get_org_from_internal_symbol(badge_symbols[i], failure_identifier), failure_identifier + "Org mismatch for badge " + badge_symbols[i].code().to_string());
    }
    action {
      permission_level{get_self(), name("active")},
      name(BOUNDED_AGG_CONTRACT),
      name("resumebadges"),
      resumebadges_args {
        .org = org,
        .agg_symbol = agg_symbol,
        .seq_id = seq_id,
        .badge_symbols = badge_symbols
      }
    }.send();
}


ACTION bamanager::addaggauth(name org, name action, name agg, name authorized_account) {
    require_auth(org); // Ensure the organization making the change is authorized

    aggauths_table aggauths(get_self(), org.value); // Access the table scoped to the organization
    auto secondary_index = aggauths.get_index<"byactionagg"_n>(); // Access the secondary index
    uint128_t secondary_key = aggauths::combine_names(action, agg); // Combine action and agg names into a single secondary key
    auto itr = secondary_index.find(secondary_key); // Find the record using the secondary key

    if (itr != secondary_index.end() && itr->action == action && itr->agg == agg) {
        // Ensure the account is not already authorized before modifying
        bool is_account_already_authorized = std::find(itr->authorized_accounts.begin(), itr->authorized_accounts.end(), authorized_account) != itr->authorized_accounts.end();
        check(!is_account_already_authorized, "Account is already authorized for this agg and action.");

        aggauths.modify(*itr, get_self(), [&](auto& row) { // Note: Use aggauths.modify not secondary_index.modify
            row.authorized_accounts.push_back(authorized_account); // Add the account
        });
    } else {
        aggauths.emplace(get_self(), [&](auto& row) { // If the record does not exist, create a new one
            row.id = aggauths.available_primary_key(); // Assign a new primary key
            row.action = action;
            row.agg = agg;
            row.authorized_accounts.push_back(authorized_account); // Add the account
        });
    }
}


ACTION bamanager::delaggauth(name org, name action, name agg, name authorized_account) {
    require_auth (org);

    string action_name = "delaggauth";
    string failure_identifier = "CONTRACT: bamanager, ACTION: " + action_name + ", MESSAGE: ";
 
    aggauths_table aggauths(get_self(), org.value);
    auto secondary_index = aggauths.get_index<"byactionagg"_n>();
    uint128_t secondary_key = aggauths::combine_names(action, agg);
    auto itr = secondary_index.find(secondary_key);

    check(itr != secondary_index.end() && itr->action == action && itr->agg == agg, failure_identifier + "agg with specified action and agg not found");

    auto& accounts = itr->authorized_accounts;
    auto acc_itr = std::find(accounts.begin(), accounts.end(), authorized_account);
    check(acc_itr != accounts.end(), failure_identifier + "Account not found in authorized accounts");
    bool should_erase = false;
    secondary_index.modify(itr, get_self(), [&](auto& row) {
        row.authorized_accounts.erase(acc_itr);

        should_erase = row.authorized_accounts.empty();
    });
    if (should_erase) {
        secondary_index.erase(itr); // Erase the row from the table
    }
}

ACTION bamanager::addactionauth(name org, name action, name authorized_account) {
    require_auth(org); // Ensure that the organization is authorized to make this change

    actionauths_table _actionauths(get_self(), org.value); // Access the table scoped to the organization
    auto itr = _actionauths.find(action.value); // Find the action in the table

    if (itr != _actionauths.end()) { // Check if the action exists
        // Modify the found row
        _actionauths.modify(itr, get_self(), [&](auto& row) {
            // Check if the authorized_account is already in the vector
            auto it = std::find(row.authorized_accounts.begin(), row.authorized_accounts.end(), authorized_account);
            if (it == row.authorized_accounts.end()) { // If the account is not found
                row.authorized_accounts.push_back(authorized_account); // Add the account
            } else {
                // If the account is found, throw an error
                check(false, "Account is already authorized for this action.");
            }
        });
    } else {
        // If the action does not exist, create a new row
        _actionauths.emplace(get_self(), [&](auto& row) {
            row.action = action;
            row.authorized_accounts.push_back(authorized_account); // Add the account
        });
    }
}


ACTION bamanager::delactionauth(name org, name action, name authorized_account) {
    require_auth(org); // Ensure that the organization is authorized to make this change

    actionauths_table _actionauths(get_self(), org.value); // Access the table scoped to the organization
    auto itr = _actionauths.find(action.value); // Find the action in the table

    if (itr != _actionauths.end()) { // Check if the action exists
        // Temporary flag to indicate whether to erase the row
        bool should_erase = false;

        _actionauths.modify(itr, get_self(), [&](auto& row) { // Modify the found row
            auto it = std::find(row.authorized_accounts.begin(), row.authorized_accounts.end(), authorized_account); // Find the account to delete
            if (it != row.authorized_accounts.end()) { // If the account is found
                row.authorized_accounts.erase(it); // Erase the account from the vector
            }
            
            // Set flag if the vector is empty after deletion
            should_erase = row.authorized_accounts.empty();
        });

        // If the vector is empty after modification, erase the row
        if (should_erase) {
            _actionauths.erase(itr); // Erase the row from the table
        }
    }
    check(false, "account already not authorized");
}
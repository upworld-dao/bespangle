#include <org.hpp>
#include <json.hpp>
using json = nlohmann::json;

ACTION org::chkscontract(name org, name checks_contract) {
  require_auth(org);
  orgs_index _orgs(get_self(), get_self().value);
  auto itr = _orgs.find(org.value);
  check(itr != _orgs.end(), "Organization does not exist");

  _orgs.modify(itr, org, [&](auto& row) {
    row.checks_contract = checks_contract;
  });

}


ACTION org::image(name org, string ipfs_image) {
  // Check if the caller is authorized for this action
  name current_account = get_first_receiver();
  check(is_action_authorized(org, "image"_n, current_account), "Account not authorized for this action");
    
  // Open the table with the contract's scope
  orgs_index _orgs(get_self(), get_self().value);
  auto itr = _orgs.find(org.value);
  check(itr != _orgs.end(), "Organization does not exist");

  // Parse the existing offchain JSON data
  nlohmann::json offchain_json;
  if (itr->offchain_lookup_data.empty()) {
      offchain_json = nlohmann::json::object();
  } else {
      check(nlohmann::json::accept(itr->offchain_lookup_data), "Stored JSON is non-empty and not valid");
      offchain_json = nlohmann::json::parse(itr->offchain_lookup_data);
  }

  // Ensure there is a "user" object in the JSON.
  if (offchain_json.find("user") == offchain_json.end() || !offchain_json["user"].is_object()) {
      offchain_json["user"] = nlohmann::json::object();
  }

  // Update the "ipfs" field within the "user" object.
  offchain_json["user"]["ipfs_image"] = ipfs_image;

  // Modify the record in the table with the updated JSON
  _orgs.modify(itr, same_payer, [&](auto& row) {
      row.offchain_lookup_data = offchain_json.dump();
  });
}

ACTION org::initorg(name org, string org_code, string ipfs_image, string display_name) {
  require_auth(org);

  // Validate org_code: must be exactly 4 lowercase alphabetic characters.
  check(org_code.size() == 4, "org_code must be exactly 4 characters long");
  check(std::all_of(org_code.begin(), org_code.end(), [](char c) {
      return std::isalpha(c) && std::islower(c);
  }), "org_code must only contain lowercase alphabets");

  // Convert the validated string org_code to eosio::name.
  name converted_org_code = name(org_code);

  // Open the table (scope is the contract itself).
  orgs_index _orgs(get_self(), get_self().value);

  // Ensure org_code is unique using the secondary index.
  auto org_code_index = _orgs.get_index<"orgsidx"_n>();
  auto org_code_iterator = org_code_index.find(converted_org_code.value);
  check(org_code_iterator == org_code_index.end() || org_code_iterator->org_code != converted_org_code,
        "org_code must be unique");

  // Ensure the organization (primary key) does not already exist.
  auto itr = _orgs.find(org.value);
  check(itr == _orgs.end(), "Organization already exists");

  // Build offchain JSON: store ipfs_image under the "user" tag.
  nlohmann::json offchain_json;
  offchain_json["user"] = {{"ipfs_image", ipfs_image}};

  // Build onchain JSON:
  //   Under the "user" tag, store the display_name.
  //   Under the "system" tag, store the created_at timestamp.
  nlohmann::json onchain_json;
  onchain_json["user"]   = {{"display_name", display_name}};
  uint32_t current_time  = current_time_point().sec_since_epoch();
  onchain_json["system"] = {{"created_at", current_time}};

  // Insert new record into the table.
  _orgs.emplace(get_self(), [&](auto& row) {
      row.org                  = org;
      row.org_code             = converted_org_code;
      row.offchain_lookup_data = offchain_json.dump();
      row.onchain_lookup_data  = onchain_json.dump();
  });

  // Send an inline action to SUBSCRIPTION_CONTRACT (assuming haspackage_args and SUBSCRIPTION_CONTRACT are defined).
  action{
      permission_level{get_self(), name("active")},
      name(SUBSCRIPTION_CONTRACT),
      name("haspackage"),
      std::make_tuple(org)
  }.send();
}



ACTION org::displayname(name org, string display_name) {
  // Require that the org account authorizes the update.
  require_auth(org);

  // Open the table with the contract's scope.
  orgs_index _orgs(get_self(), get_self().value);
  auto itr = _orgs.find(org.value);
  check(itr != _orgs.end(), "Organization does not exist");

  // Parse the existing onchain JSON data.
  nlohmann::json onchain_json;
  if (itr->onchain_lookup_data.empty()) {
      onchain_json = nlohmann::json::object();
  } else {
      check(nlohmann::json::accept(itr->onchain_lookup_data), "Stored onchain_lookup_data is not valid JSON");
      onchain_json = nlohmann::json::parse(itr->onchain_lookup_data);
      check(onchain_json.is_object(), "Stored onchain_lookup_data must be a JSON object");
  }

  // Ensure that the "user" object exists.
  if (onchain_json.find("user") == onchain_json.end() || !onchain_json["user"].is_object()) {
      onchain_json["user"] = nlohmann::json::object();
  }

  // Update the "name" field inside the "user" object.
  onchain_json["user"]["display_name"] = display_name;

  // Write the updated JSON back to the table.
  _orgs.modify(itr, same_payer, [&](auto& row) {
      row.onchain_lookup_data = onchain_json.dump();
  });
}

ACTION org::offckeyvalue(name org, string key, string value) {
  // Require authorization from the org account.
  require_auth(org);

  put_offchain_key_value(org, key, value);

}

ACTION org::onckeyvalue(name org, string key, string value) {
  // Require that the org account authorizes the update.
  require_auth(org);

  put_onchain_key_value(org, key, value);
}

ACTION org::addactionauth(name org, name action, name authorized_account) {
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

ACTION org::delactionauth(name org, name action, name authorized_account) {
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
}

ACTION org::nextbadge(name org) {
  // Setup action name and error message prefix.
  string action_name = "nextbadge";
  string failure_identifier = "CONTRACT: org, ACTION: " + action_name + ", MESSAGE: ";
  
  // Verify internal authorization.
  check_internal_auth(get_self(), name(action_name), failure_identifier);

  // Retrieve the organization record.
  orgs_index _orgs(get_self(), get_self().value);
  auto itr = _orgs.find(org.value);
  check(itr != _orgs.end(), failure_identifier + "org not found");
  
  // Get the organization code.
  string org_code = itr->org_code.to_string();

  // Parse the onchain_lookup_data JSON.
  nlohmann::json onchain;
  if (itr->onchain_lookup_data.empty()) {
      onchain = nlohmann::json::object();
  } else {
      check(nlohmann::json::accept(itr->onchain_lookup_data), failure_identifier + "onchain_lookup_data invalid JSON");
      onchain = nlohmann::json::parse(itr->onchain_lookup_data);
      check(onchain.is_object(), failure_identifier + "onchain_lookup_data not a JSON object");
  }

  // Get or create the "system" object within the onchain JSON.
  nlohmann::json system_obj;
  if (onchain.find("system") != onchain.end() && onchain["system"].is_object()) {
      system_obj = onchain["system"];
  } else {
      system_obj = nlohmann::json::object();
  }

  // Retrieve the current nextbadge code from the system object; default to "aaa".
  string next_code = "aaa";
  if (system_obj.find("nextbadge") != system_obj.end()) {
      next_code = system_obj["nextbadge"].get<string>();
  }

  // While the emission symbol (constructed from org_code + next_code) exists, increment next_code.
  while (badge_exists(symbol(org_code + next_code, 0), org)) {
      next_code = increment_auto_code(next_code);
  }

  // Update the system object with the new nextbadge code.
  system_obj["nextbadge"] = next_code;
  onchain["system"] = system_obj;

  // Persist the updated onchain_lookup_data back into the organization record.
  _orgs.modify(itr, same_payer, [&](auto& row) {
      row.onchain_lookup_data = onchain.dump();
  });
}


ACTION org::nextemission(name org) {
  // Setup action name and failure identifier for error messages.
  string action_name = "nextemission";
  string failure_identifier = "CONTRACT: org, ACTION: " + action_name + ", MESSAGE: ";
  
  // Ensure internal authorization.
  check_internal_auth(get_self(), name(action_name), failure_identifier);

  // Retrieve the organization record.
  orgs_index _orgs(get_self(), get_self().value);
  auto itr = _orgs.find(org.value);
  check(itr != _orgs.end(), failure_identifier + "org not found");
  
  // Get the organization's code as a string.
  string org_code = itr->org_code.to_string();

  // Parse the offchain_lookup_data, which is expected to be a JSON string.
  nlohmann::json onchain;
  if (itr->onchain_lookup_data.empty()) {
      onchain = nlohmann::json::object();
  } else {
      check(nlohmann::json::accept(itr->onchain_lookup_data), failure_identifier + "onchain_lookup_data invalid JSON");
      onchain = nlohmann::json::parse(itr->onchain_lookup_data);
      check(onchain.is_object(), failure_identifier + "onchain_lookup_data not a JSON object");
  }

  // Get or create the "system" object within the onchain JSON.
  nlohmann::json system_obj;
  if (onchain.find("system") != onchain.end() && onchain["system"].is_object()) {
      system_obj = onchain["system"];
  } else {
      system_obj = nlohmann::json::object();
  }

  // Retrieve the current nextbadge code from the system object; default to "aaa".
  string next_code = "aaa";
  if (system_obj.find("nextemission") != system_obj.end()) {
      next_code = system_obj["nextemission"].get<string>();
  }

  // While the emission symbol (constructed from org_code + next_code) exists, increment next_code.
  while (emission_exists(symbol(org_code + next_code, 0), org)) {
      next_code = increment_auto_code(next_code);
  }

  // Update the system object with the new nextemission code.
  system_obj["nextemission"] = next_code;
  onchain["system"] = system_obj;

  // Persist the updated onchain_lookup_data back into the organization record.
  _orgs.modify(itr, same_payer, [&](auto& row) {
      row.onchain_lookup_data = onchain.dump();
  });
}
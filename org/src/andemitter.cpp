#include <andemitter.hpp>
#include <json.hpp>
using json = nlohmann::json;

void andemitter::notifyachiev(name org, asset amount, name from, name to, string memo, vector<name> notify_accounts) {
    string action_name = "notifyachiev";
    string failure_identifier = "CONTRACT: andemitter, ACTION: " + action_name + ", MESSAGE: ";
    
    activelookup_table active_lookup(get_self(), get_self().value);
    auto lookup_itr = active_lookup.find(amount.symbol.code().raw());
    if(lookup_itr == active_lookup.end()) {
        return;
    }

    emissions_table emissions(get_self(), org.value);
    accounts_table accounts(get_self(), to.value);

    uint8_t actions_used = 0;
    for (const auto& emission_symbol : lookup_itr->active_emissions) {
        auto emission_itr = emissions.find(emission_symbol.code().raw());
        check(emission_itr != emissions.end(), "Emission does not exist");

        auto account_itr = accounts.find(emission_symbol.code().raw());
        uint8_t emission_status = emission_itr->cyclic ? CYCLIC_IN_PROGRESS : NON_CYCLIC_IN_PROGRESS;

        if (account_itr != accounts.end() && account_itr->emission_status == NON_CYCLIC_EMITTED) continue;

        if (account_itr == accounts.end()) {
            account_itr = accounts.emplace(get_self(), [&](auto& acc) {
                acc.emission_symbol = emission_symbol;
                acc.emission_status = emission_status;
            });
        } else {
            // Modification logic commented out
            // accounts.modify(account_itr, same_payer, [&](auto& acc) {
            //     acc.expanded_emitter_status[amount.symbol.code()] += amount.amount;
            // });
        }
        auto expanded_emitter_status_map = account_itr->expanded_emitter_status;
        update_expanded_emitter_status(to, expanded_emitter_status_map, amount, *emission_itr, emission_status, failure_identifier);

        accounts.modify(account_itr, get_self(), [&](auto& acc) {
            acc.expanded_emitter_status = expanded_emitter_status_map;
            acc.emission_status = emission_status;
        });
        actions_used++;
    }

    if (actions_used > 0) {
        action {
            permission_level{get_self(), name("active")},
            name(SUBSCRIPTION_CONTRACT),
            name("billing"),
            billing_args {
                .org = org,
                .actions_used = actions_used}
        }.send();
    }

}

ACTION andemitter::newemission( name org,
                    symbol emission_symbol,
                    string display_name, 
                    string ipfs_description,
                    vector<asset> emitter_criteria,
                    vector<contract_asset> emit_assets,
                    bool cyclic ) {
    // Internal authorization check.
    string action_name = "newemission";
    string failure_identifier = "CONTRACT: andemitter, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    // Open the emissions table (scope is org.value).
    emissions_table emissions(get_self(), org.value);
    auto emission_itr = emissions.find(emission_symbol.code().raw());
    check(emission_itr == emissions.end(), "Emission already exists");

    // Build the offchain JSON.
    // Under "user": store the ipfs_description using the key "ipfs_description".
    nlohmann::json offchain_json;
    offchain_json["user"] = { {"ipfs_description", ipfs_description} };

    // Build the onchain JSON.
    // Under "user": store the display_name using the key "name".
    // Under "system": store the created_at timestamp.
    nlohmann::json onchain_json;
    onchain_json["user"] = { {"name", display_name} };
    uint32_t created_at = current_time_point().sec_since_epoch();
    onchain_json["system"] = { {"created_at", created_at} };

    // Convert the vector<asset> emitter_criteria to a map<symbol_code, asset>.
    map<symbol_code, asset> emitter_criteria_map;
    for (const auto& crit : emitter_criteria) {
        emitter_criteria_map[crit.symbol.code()] = crit;
    }

    // Insert the new emission record.
    emissions.emplace(get_self(), [&](auto& em) {
        em.emission_symbol         = emission_symbol;
        em.offchain_lookup_data    = offchain_json.dump();
        em.onchain_lookup_data     = onchain_json.dump();
        em.emitter_criteria        = emitter_criteria_map;
        em.emit_assets             = emit_assets;
        em.status                  = name("init");
        em.cyclic                 = cyclic;
    });

    action {
        permission_level{get_self(), name("active")},
        name(SUBSCRIPTION_CONTRACT),
        name("billing"),
        billing_args {
            .org = org,
            .actions_used = 1}
    }.send();
}

ACTION andemitter::activate(name org, symbol emission_symbol) {
    string action_name = "activate";
    string failure_identifier = "CONTRACT: andemitter, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier); 

    emissions_table emissions(get_self(), org.value);
    auto emission_itr = emissions.find(emission_symbol.code().raw());
    check(emission_itr != emissions.end(), "Emission does not exist");

    emissions.modify(emission_itr, get_self(), [&](auto& mod) {
        mod.status = name("activate");
    });

    activelookup_table activelookup(get_self(), get_self().value);
    for (const auto& rec : emission_itr->emitter_criteria) {
        auto badge_symbol = rec.second.symbol;
        auto lookup_itr = activelookup.find(badge_symbol.code().raw());

        if (lookup_itr == activelookup.end()) {
            activelookup.emplace(get_self(), [&](auto& new_entry) {
                new_entry.badge_symbol = badge_symbol;
                new_entry.active_emissions.push_back(emission_symbol);
            });
        } else {
            activelookup.modify(lookup_itr, get_self(), [&](auto& mod_entry) {
                mod_entry.active_emissions.push_back(emission_symbol);
            });
        }
    }

}

ACTION andemitter::deactivate(name org, symbol emission_symbol) {
    string action_name = "deactivate";
    string failure_identifier = "CONTRACT: andemitter, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier); 

    emissions_table emissions(get_self(), org.value);
    auto emission_itr = emissions.find(emission_symbol.code().raw());
    check(emission_itr != emissions.end(), "Emission does not exist");

    emissions.modify(emission_itr, get_self(), [&](auto& mod) {
        mod.status = name("deactivate");
    });

    activelookup_table activelookup(get_self(), get_self().value);
    for (const auto& rec : emission_itr->emitter_criteria) {
        auto badge_symbol = rec.second.symbol;
        auto lookup_itr = activelookup.find(badge_symbol.code().raw());

        if (lookup_itr != activelookup.end()) {
            auto& active_emissions = lookup_itr->active_emissions;
            auto it = std::find(active_emissions.begin(), active_emissions.end(), emission_symbol);

            if (it != active_emissions.end()) {
                activelookup.modify(lookup_itr, get_self(), [&](auto& mod) {
                    mod.active_emissions.erase(it);
                });

                if (lookup_itr->active_emissions.empty()) {
                    activelookup.erase(lookup_itr);
                }
            }
        }
    }
}

ACTION andemitter::offckeyvalue(name org, symbol emission_symbol, string key, string value) {
    string action_name = "offckeyvalue";
    string failure_identifier = "CONTRACT: andemitter, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    // Open the emissions table.
    emissions_table emissions(get_self(), org.value);
    auto itr = emissions.find(emission_symbol.code().raw());
    check(itr != emissions.end(), "Emission does not exist");

    // Parse the current offchain_lookup_data.
    nlohmann::json j;
    if (itr->offchain_lookup_data.empty()) {
        j = nlohmann::json::object();
    } else if (nlohmann::json::accept(itr->offchain_lookup_data)) {
        j = nlohmann::json::parse(itr->offchain_lookup_data);
    } else {
        check(false, "Stored offchain_lookup_data is not valid JSON");
    }
    check(j.is_object(), "offchain_lookup_data must be a JSON object");

    // Ensure the "user" object exists.
    if (j.find("user") == j.end() || !j["user"].is_object()) {
        j["user"] = nlohmann::json::object();
    }

    // Restrictions:
    // 1. "system" tag cannot be modified.
    check(key != "system", "Cannot modify system tag in offchain lookup");

    // Update or insert the key/value pair under the "user" tag.
    j["user"][key] = value;

    // Save the updated JSON back to the badge record.
    emissions.modify(itr, get_self(), [&](auto& row) {
        row.offchain_lookup_data = j.dump();
    });
}

ACTION andemitter::onckeyvalue(name org, symbol emission_symbol, string key, string value) {
    string action_name = "onckeyvalue";
    string failure_identifier = "CONTRACT: andemitter, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    // Open the badge table.
    emissions_table emissions(get_self(), org.value);
    auto itr = emissions.find(emission_symbol.code().raw());
    check(itr != emissions.end(), "Emission does not exist");

    // Parse the current onchain_lookup_data.
    nlohmann::json j;
    if (itr->onchain_lookup_data.empty()) {
        j = nlohmann::json::object();
    } else if (nlohmann::json::accept(itr->onchain_lookup_data)) {
        j = nlohmann::json::parse(itr->onchain_lookup_data);
    } else {
        check(false, "Stored onchain_lookup_data is not valid JSON");
    }
    check(j.is_object(), "onchain_lookup_data must be a JSON object");

    // Ensure the "user" object exists.
    if (j.find("user") == j.end() || !j["user"].is_object()) {
        j["user"] = nlohmann::json::object();
    }

    // Restrictions:
    // 1. "system" tag cannot be modified.
    check(key != "system", "Cannot modify system tag in onchain lookup");
    // Update or insert the key/value pair under the "user" tag.
    j["user"][key] = value;

    // Save the updated JSON back to the badge record.
    emissions.modify(itr, get_self(), [&](auto& row) {
        row.onchain_lookup_data = j.dump();
    });
} 



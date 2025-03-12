#include <badgedata.hpp>
#include <json.hpp>
using json = nlohmann::json;


ACTION badgedata::initbadge(name org,
                symbol badge_symbol, 
                string display_name, 
                string ipfs_image,
                string description, 
                string memo) {
    
    string action_name = "initbadge";
    string failure_identifier = "CONTRACT: badgedata, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    // Open the badge table.
    badge_table badges(get_self(), org.value);

    // Ensure a badge with the same symbol does not already exist.
    auto badge_itr = badges.find(badge_symbol.code().raw());
    check(badge_itr == badges.end(), "Badge already exists");

    // Build the onchain JSON.
    // Under "user": store display_name (as "name") and description (as "description").
    // Under "system": store created_at timestamp.
    nlohmann::json onchain_json;
    onchain_json["user"] = {
        {"display_name", display_name},
        {"description", description}
    };
    uint32_t created_at = current_time_point().sec_since_epoch();
    onchain_json["system"] = {
        {"created_at", created_at}
    };

    // Build the offchain JSON.
    // Under "user": store ipfs_image (as "ipfs_image").
    nlohmann::json offchain_json;
    offchain_json["user"] = {
        {"ipfs_image", ipfs_image}
    };

    // Insert the new badge record.
    badges.emplace(get_self(), [&](auto &row) {
        row.badge_symbol         = badge_symbol;
        row.notify_accounts      = vector<name>(); // Start with an empty list.
        row.offchain_lookup_data = offchain_json.dump();
        row.onchain_lookup_data  = onchain_json.dump();
        row.rarity_counts        = 0;
    });
}

ACTION badgedata::addfeature(
    name org, 
    symbol badge_symbol, 
    name notify_account, 
    string memo) {

    string action_name = "addfeature";
    string failure_identifier = "CONTRACT: badgedata, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    badge_table _badge(get_self(), org.value);

    auto badge_iterator = _badge.find(badge_symbol.code().raw());
    check(badge_iterator != _badge.end(), failure_identifier + " badge_symbol not initialized ");

    vector<name> new_notify_accounts;
    for (auto i = 0; i < badge_iterator->notify_accounts.size(); i++) {
        if (notify_account == badge_iterator->notify_accounts[i]) {
            return;
        }
        new_notify_accounts.push_back(badge_iterator->notify_accounts[i]);
    }
    new_notify_accounts.push_back(notify_account);
    _badge.modify(badge_iterator, get_self(), [&](auto& row) {
        row.notify_accounts = new_notify_accounts;
    });

    action {
        permission_level{get_self(), name("active")},
        get_self(),
        name("addnotify"),
        downstream_notify_args {
            .org = org,
            .badge_symbol = badge_symbol,
            .notify_account = notify_account,
            .memo = memo,
            .offchain_lookup_data = badge_iterator->offchain_lookup_data,
            .onchain_lookup_data = badge_iterator->onchain_lookup_data,
            .rarity_counts = badge_iterator->rarity_counts
        }
    }.send();
}

ACTION badgedata::addnotify(
    name org,
    symbol badge_symbol, 
    name notify_account, 
    string memo, 
    string offchain_lookup_data, 
    string onchain_lookup_data, 
    uint64_t rarity_counts) {

    require_auth(get_self());
    require_recipient(notify_account);
}

ACTION badgedata::delfeature(
    name org,
    symbol badge_symbol, 
    name notify_account, 
    string memo) {

    string action_name = "delfeature";
    string failure_identifier = "CONTRACT: badgedata, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    badge_table _badge(get_self(), org.value);

    auto badge_iterator = _badge.find(badge_symbol.code().raw());
    check(badge_iterator != _badge.end(), failure_identifier + " badge_symbol not initialized ");

    vector<name> new_notify_accounts;
    for (auto i = 0; i < badge_iterator->notify_accounts.size(); i++) {
        if (notify_account != badge_iterator->notify_accounts[i]) {
            new_notify_accounts.push_back(badge_iterator->notify_accounts[i]);
        }
    }
    _badge.modify(badge_iterator, get_self(), [&](auto& row) {
        row.notify_accounts = new_notify_accounts;
    });

    action {
        permission_level{get_self(), name("active")},
        get_self(),
        name("delnotify"),
        downstream_notify_args {
            .org = org,
            .badge_symbol = badge_symbol,
            .notify_account = notify_account,
            .memo = memo,
            .offchain_lookup_data = badge_iterator->offchain_lookup_data,
            .onchain_lookup_data = badge_iterator->onchain_lookup_data,
            .rarity_counts = badge_iterator->rarity_counts
        }
    }.send();
}

ACTION badgedata::delnotify(
    name org,
    symbol badge_symbol, 
    name notify_account, 
    string memo, 
    string offchain_lookup_data, 
    string onchain_lookup_data, 
    uint64_t rarity_counts) {

    require_auth(get_self());
    require_recipient(notify_account);

}
ACTION badgedata::achievement(
    name org,
    asset badge_asset, 
    name from,
    name to, 
    string memo) {

    string action_name = "achievement";
    string failure_identifier = "CONTRACT: badgedata, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    badge_table _badge(get_self(), org.value);

    auto badge_iterator = _badge.find(badge_asset.symbol.code().raw());
    check(badge_iterator != _badge.end(), failure_identifier + " symbol for asset not initialized ");

    _badge.modify(badge_iterator, get_self(), [&](auto& row) {
        row.rarity_counts += badge_asset.amount;
    });

    action {
        permission_level{get_self(), name("active")},
        get_self(),
        name("notifyachiev"),
        notifyachievement_args {
            .org = org,
            .badge_asset = badge_asset,
            .from = from,
            .to = to,
            .memo = memo,
            .notify_accounts = badge_iterator->notify_accounts
        }
    }.send();    
}

ACTION badgedata::notifyachiev(
    name org,
    asset badge_asset, 
    name from, 
    name to, 
    string memo, 
    vector<name> notify_accounts) {

    require_auth(get_self());
    for (auto& notify_account : notify_accounts) {
        require_recipient(notify_account);
    }

}

ACTION badgedata::offckeyvalue(name org, symbol badge_symbol, string key, string value) {
    string action_name = "offckeyvalue";
    string failure_identifier = "CONTRACT: badgedata, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    // Open the badge table.
    badge_table badges(get_self(), org.value);
    auto itr = badges.find(badge_symbol.code().raw());
    check(itr != badges.end(), "Badge does not exist");

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
    // (Note: "name" and "description" tags are allowed offchain.)

    // Update or insert the key/value pair under the "user" tag.
    j["user"][key] = value;

    // Save the updated JSON back to the badge record.
    badges.modify(itr, get_self(), [&](auto& row) {
        row.offchain_lookup_data = j.dump();
    });
}

ACTION badgedata::onckeyvalue(name org, symbol badge_symbol, string key, string value) {
    string action_name = "onckeyvalue";
    string failure_identifier = "CONTRACT: badgedata, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    // Open the badge table.
    badge_table badges(get_self(), org.value);
    auto itr = badges.find(badge_symbol.code().raw());
    check(itr != badges.end(), "Badge does not exist");

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
    badges.modify(itr, get_self(), [&](auto& row) {
        row.onchain_lookup_data = j.dump();
    });
} 



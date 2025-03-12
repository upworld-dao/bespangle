#include <subscription.hpp>

// todo cache actions usage
ACTION subscription::billing(name org, uint8_t actions_used) {
    string action_name = "billing";
    string failure_identifier = "CONTRACT: subscription, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    // Open the orgpackage table scoped by the organization
    orgpackage_table orgpackages(get_self(), org.value);

    // Access the current package
    auto status_index = orgpackages.get_index<"bystatus"_n>();
    auto current_itr = status_index.find(name("current").value);
    time_point_sec current_time = eosio::current_time_point();
    // Check if a current package does not exist, is expired, or is exhausted
    if (current_itr == status_index.end() ||
        current_time > current_itr->expiry_time ||
        current_itr->actions_used >= current_itr->total_actions_bought ||
        current_itr->status != name("current")) {
        // Handle expired or missing current package
        auto new_itr = status_index.find(name("new").value);
        check(new_itr != status_index.end(), "No NEW package available to update");

        // Promote the NEW package to CURRENT
        status_index.modify(new_itr, get_self(), [&](auto& new_pkg) {
            new_pkg.status = "current"_n;
            new_pkg.actions_used = actions_used; 
            new_pkg.expiry_time = time_point_sec(current_time_point().sec_since_epoch() +
                                                 new_pkg.expiry_duration_in_secs);
        });

        // Update the old current package to USED if it exists
        if (current_itr != status_index.end() && current_itr->status == name("current")) {
            status_index.modify(current_itr, get_self(), [&](auto& old_pkg) {
                old_pkg.status = "used"_n;
            });
        }
    } else {
        // Increment actions used for the current package
        status_index.modify(current_itr, get_self(), [&](auto& pkg) {
            pkg.actions_used += actions_used;
        });
    }    

}

void subscription::buypack(name from, name to, asset amount, std::string memo) {
    if (from == get_self() || to != get_self()) return;

    check(amount.amount > 0, "Transfer amount must be positive");

    auto [org, package_name] = parse_memo(memo);

    handle_pack_purchase(org, package_name, amount, from, get_first_receiver());
}

ACTION subscription::newpack(
    name package,
    string descriptive_name,
    uint64_t action_size,
    uint64_t expiry_duration_in_secs,
    extended_asset cost,
    bool active,
    bool display) {
    require_auth(get_self()); // Ensure action is authorized by the contract account

    // Validate inputs
    check(package.length() > 0, "Package name cannot be empty");
    check(descriptive_name.size() > 0, "Descriptive name cannot be empty");
    check(action_size > 0, "Action size must be greater than zero");
    check(expiry_duration_in_secs > 0, "Expiry duration must be greater than zero");
    check(cost.quantity.amount >= 0, "Cost cannot have a negative amount");

    // Access the table
    packages_table _packages(get_self(), get_self().value);

    // Ensure the package name does not already exist
    auto itr = _packages.find(package.value);
    check(itr == _packages.end(), "Package with this name already exists");

    // Insert new record into the table
    _packages.emplace(get_self(), [&](auto& row) {
        row.package = package;
        row.descriptive_name = descriptive_name;
        row.action_size = action_size;
        row.expiry_duration_in_secs = expiry_duration_in_secs;
        row.cost = cost;
        row.active = active;
        row.display = display;
    });
}

ACTION subscription::disablepack(name package) {
    require_auth(get_self()); // Ensure only the contract owner can disable a package

    // Open the packages table
    packages_table packages(get_self(), get_self().value);

    // Find the package by its primary key
    auto package_itr = packages.find(package.value);
    check(package_itr != packages.end(), "Package not found.");

    // Modify the record to set active to false
    packages.modify(package_itr, get_self(), [&](auto& row) {
        row.active = false;
    });
}

ACTION subscription::enablepack(name package) {
    require_auth(get_self()); // Ensure only the contract owner can enable a package

    // Open the packages table
    packages_table packages(get_self(), get_self().value);

    // Find the package by its primary key
    auto package_itr = packages.find(package.value);
    check(package_itr != packages.end(), "Package not found.");

    // Modify the record to set active to true
    packages.modify(package_itr, get_self(), [&](auto& row) {
        row.active = true;
    });
}

ACTION subscription::enableui(name package) {
    require_auth(get_self()); // Ensure only the contract owner can enable a package

    // Open the packages table
    packages_table packages(get_self(), get_self().value);

    // Find the package by its primary key
    auto package_itr = packages.find(package.value);
    check(package_itr != packages.end(), "Package not found.");

    // Modify the record to set active to true
    packages.modify(package_itr, get_self(), [&](auto& row) {
        row.display = true;
    });
}

ACTION subscription::disableui(name package) {
    require_auth(get_self()); // Ensure only the contract owner can enable a package

    // Open the packages table
    packages_table packages(get_self(), get_self().value);

    // Find the package by its primary key
    auto package_itr = packages.find(package.value);
    check(package_itr != packages.end(), "Package not found.");

    // Modify the record to set active to true
    packages.modify(package_itr, get_self(), [&](auto& row) {
        row.display = false;
    });
}

ACTION subscription::haspackage(name org) {

    // Define the orgpackage table scoped by the given org
    orgpackage_table packages(get_self(), org.value);

    // Check if there is at least one record in the table
    auto itr = packages.begin();
    check(itr != packages.end(), "No package record found for the organization.");

}

ACTION subscription::resetseqid(name key, uint64_t new_value) {
      // Only the contract account is allowed to reset or initialize sequence IDs.
      require_auth(get_self());

      // Instantiate the multi-index table for sequences.
      sequences_table _seq(get_self(), get_self().value);

      // Attempt to find the record by its key.
      auto itr = _seq.find(key.value);

      // If the record doesn't exist, insert a new one.
      if (itr == _seq.end()) {
         _seq.emplace(get_self(), [&](auto& row) {
            row.key = key;
            row.last_used_value = new_value;
         });
      } else {
         // Otherwise, update the existing record with the new value.
         _seq.modify(itr, same_payer, [&](auto& row) {
            row.last_used_value = new_value;
         });
      }
   }


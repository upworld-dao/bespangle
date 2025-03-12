#include <authority.hpp>

ACTION authority::addauth(name contract, name action, name authorized_contract) {
    require_auth(get_self()); // Ensure this action is authorized by the contract itself

    auth_table auths(get_self(), contract.value); // Access the auth table scoped by the contract

    auto itr = auths.find(action.value); // Find the record for the specified action
    if (itr != auths.end()) {
        // Check if authorized_contract is already in the list
        auto& auth_contracts = itr->authorized_contracts;
        if (std::find(auth_contracts.begin(), auth_contracts.end(), authorized_contract) != auth_contracts.end()) {
            eosio::print("Contract already authorized");
        } else {
            // Modify the record to add the new authorized_contract
            auths.modify(itr, get_self(), [&](auto& auth) {
                auth.authorized_contracts.push_back(authorized_contract);
            });
        }
    } else {
        // Create a new record if it doesn't exist
        auths.emplace(get_self(), [&](auto& auth) {
            auth.action = action;
            auth.authorized_contracts.push_back(authorized_contract);
        });
    }
}


// Remove a single authorized contract based on contract and action
ACTION authority::removeauth(name contract, name action, name authorized_contract) {
    require_auth(get_self()); // Ensure this action is authorized by the contract itself

    auth_table auths(get_self(), contract.value); // Access the auth table scoped by the contract

    auto itr = auths.find(action.value); // Find the record for the specified action
    if (itr != auths.end()) {
        // Modify the record to remove the specified authorized_contract
        auths.modify(itr, get_self(), [&](auto& auth) {
            auto& auth_contracts = auth.authorized_contracts;
            auto it = std::find(auth_contracts.begin(), auth_contracts.end(), authorized_contract);
            if (it != auth_contracts.end()) {
                auth_contracts.erase(it);
            } else {
                eosio::print("Contract not found in authorization list");
            }
        });
    } else {
        eosio::print("Action not found");
    }
}


ACTION authority::hasauth(name contract, name action, name account) {
    require_auth(get_self()); // Ensure this action is authorized by the contract itself

    auth_table auths(get_self(), contract.value); // Access the auth table scoped by the contract

    auto itr = auths.find(action.value); // Find the record for the specified action
    if (itr != auths.end()) {
        // Check if the account is in the authorized_contracts list
        if (std::find(itr->authorized_contracts.begin(), itr->authorized_contracts.end(), account) != itr->authorized_contracts.end()) {
            eosio::print("Account is authorized");
        } else {
            check(false, "Account is not authorized");
        }
    } else {
        check(false, "Action not found");
    }
}

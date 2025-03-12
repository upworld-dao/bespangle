#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <vector>
#include "authorityinterface.hpp"

using namespace eosio;
using namespace std;
using namespace authority_contract;

CONTRACT subscription : public contract {
public:
    using contract::contract;

    // Notification handlers

    [[eosio::on_notify("*::transfer")]]
    void buypack(name from, name to, asset amount, string memo);

    ACTION billing(name org, uint8_t actions_used);

    ACTION newpack(
        name package,
        string descriptive_name,
        uint64_t action_size,
        uint64_t expiry_duration_in_secs,
        extended_asset cost,
        bool active,
        bool display);

    ACTION disablepack(name package);

    ACTION enablepack(name package);

    ACTION haspackage(name org);

    ACTION enableui(name package);

    ACTION disableui(name package);

    ACTION resetseqid(name key, uint64_t new_value);

private:
    // Table definitions
    TABLE sequences {
        name key;
        uint64_t last_used_value;

        uint64_t primary_key() const { return key.value; }
    };
    typedef eosio::multi_index<"sequences"_n, sequences> sequences_table;

    TABLE packages {
        name package;
        string descriptive_name;
        uint64_t action_size;
        uint64_t expiry_duration_in_secs;
        extended_asset cost;
        bool active;
        bool display;

        uint64_t primary_key() const { return package.value; }
    };
    typedef eosio::multi_index<"packages"_n, packages> packages_table;

    TABLE orgpackage {
        uint64_t seq_id;
        name package;
        name status; // NEW, CURRENT, EXPIRED
        uint64_t total_actions_bought;
        uint64_t actions_used;
        uint64_t expiry_duration_in_secs;
        time_point_sec expiry_time;

        uint64_t primary_key() const { return seq_id; }
        uint64_t by_status() const { return status.value; }
    };
    typedef eosio::multi_index<
        "orgpackage"_n, orgpackage,
        indexed_by<"bystatus"_n, const_mem_fun<orgpackage, uint64_t, &orgpackage::by_status>>
    > orgpackage_table;



    // Utility functions
    std::tuple<name, name> parse_memo(const std::string& memo) {
        // Split the memo into parts based on the colon delimiter
        std::vector<std::string> parts;
        size_t start = 0;
        size_t end = memo.find(':');
        while (end != std::string::npos) {
            parts.push_back(memo.substr(start, end - start));
            start = end + 1;
            end = memo.find(':', start);
        }
        parts.push_back(memo.substr(start)); // Add the last part

        // Validate the number of fields
        check(parts.size() == 2 , "Invalid memo format: must have only 2 fields");

        // Extract fields
        auto org = name(parts[0]);
        auto package_name = name(parts[1]);

        return {org, package_name};
    }


    void refund_surplus(name to, asset surplus, name token_contract) {
        action(
            permission_level{get_self(), "active"_n},
            token_contract,
            "transfer"_n,
            std::make_tuple(get_self(), to, surplus, std::string("Surplus refund"))
        ).send();
    }

    void handle_pack_purchase(name org, name package_name, asset amount, name from, name first_receiver) {
        
        orgpackage_table orgpackages(get_self(), org.value);
        // Ensure only one NEW status record exists
        auto status_index = orgpackages.get_index<"bystatus"_n>();
        auto existing_new = status_index.find(name("new").value);
        check(existing_new == status_index.end(), "Another NEW status record already exists");

        // Lookup package in the packages table
        packages_table packages(get_self(), get_self().value);
        auto package_itr = packages.find(package_name.value);
        check(package_itr != packages.end(), "Package not found");
        check(package_itr->cost.quantity.symbol == amount.symbol, "Currency mismatch");
        check(package_itr->cost.contract == first_receiver, "Payment contract does not match package contract");
        check(amount >= package_itr->cost.quantity, "Insufficient funds");

        // Refund surplus
        if (amount > package_itr->cost.quantity) {
            refund_surplus(from, amount - package_itr->cost.quantity, first_receiver);
        }

        // Get the next sequence ID for this package
        uint64_t seq_id = get_next_sequence(name("packseqid"));

        // Insert new record in orgpackage table
        orgpackages.emplace(get_self(), [&](auto& row) {
            row.seq_id = seq_id;
            row.package = package_name;
            row.status = "new"_n;
            row.total_actions_bought = package_itr->action_size;
            row.actions_used = 0;
            row.expiry_duration_in_secs = package_itr->expiry_duration_in_secs;
        });
    }

    uint64_t get_next_sequence(name key) {
        sequences_table sequences(get_self(), get_self().value);

        auto itr = sequences.find(key.value);
        if (itr == sequences.end()) {
            // Initialize sequence if it does not exist
            sequences.emplace(get_self(), [&](auto& row) {
                row.key = key;
                row.last_used_value = 1;
            });
            return 1;
        } else {
            // Increment and return the next sequence value
            sequences.modify(itr, get_self(), [&](auto& row) {
                row.last_used_value += 1;
            });
            return itr->last_used_value;
        }
    }
};

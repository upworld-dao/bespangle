#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <json.hpp>
using json = nlohmann::json;


#include "badgedatainterface.hpp"
#include "authorityinterface.hpp"
#include "andemitterinterface.hpp"

using namespace std;
using namespace eosio;
using namespace badgedata_contract;
using namespace authority_contract;
using namespace andemitter_contract;

#define SUBSCRIPTION_CONTRACT "subscribedev"
#define ORG_CONTRACT "organizatdev"

CONTRACT org : public contract {
  public:
    using contract::contract;

    ACTION chkscontract (name org, name checks_contract);

    ACTION initorg(name org, string org_code, string ipfs_image, string display_name);

    ACTION image(name org, string ipfs_image);

    ACTION displayname(name org, string display_name);

    ACTION offckeyvalue(name org, string key, string value);

    ACTION onckeyvalue(name org, string key, string value);    

    ACTION nextbadge(name org);

    ACTION nextemission(name org);

    ACTION addactionauth(name org, name action, name authorized_account);

    ACTION delactionauth(name org, name action, name authorized_account);
  
  private:
    TABLE actionauths {
      name action;
      vector<name> authorized_accounts;
      auto primary_key() const { return action.value; }
    };
    typedef eosio::multi_index<"actionauths"_n, actionauths> actionauths_table;

    TABLE orgs {
        name org;         // Organization identifier, used as primary key
        name org_code;    // Converted org_code, ensuring uniqueness and specific format
        name checks_contract;
        string offchain_lookup_data;
        string onchain_lookup_data;

        // Specify the primary key
        auto primary_key() const { return org.value; }

        // Specify a secondary index for org_code to ensure its uniqueness
        uint64_t by_org_code() const { return org_code.value; }
    };

    // Declare the table
    typedef eosio::multi_index<"orgs"_n, orgs,
        eosio::indexed_by<"orgsidx"_n, eosio::const_mem_fun<orgs, uint64_t, &orgs::by_org_code>>
    > orgs_index;

    // Function to retrieve organization from symbol
    name get_org_from_internal_symbol(const symbol& _symbol, string failure_identifier) {
        string _symbol_str = _symbol.code().to_string(); // Convert symbol to string
        check(_symbol_str.size() >= 4, failure_identifier + "symbol must have at least 4 characters.");

        // Extract the first 4 characters as org_code
        string org_code_str = _symbol_str.substr(0, 4);

        for (auto & c: org_code_str) {
            c = tolower(c);
        }
        name org_code = name(org_code_str);

        // Set up the orgcode table and find the org_code
        orgs_index _orgs(name(ORG_CONTRACT), name(ORG_CONTRACT).value);
        auto org_code_itr = _orgs.get_index<"orgsidx"_n>().find(org_code.value);

        check(org_code_itr != _orgs.get_index<"orgsidx"_n>().end(), failure_identifier + "Organization code not found.");
        check(org_code_itr->org_code == org_code, failure_identifier + "Organization code not found.");
        
        // Assuming the org is stored in the same row as the org_code
        return org_code_itr->org; // Return the found organization identifier
    }

    name get_name_from_internal_symbol(const symbol& _symbol, string failure_identifier) {
        string _symbol_str = _symbol.code().to_string(); // Convert symbol to string
        check(_symbol_str.size() == 7, failure_identifier + "Symbol must have at least 7 characters.");

        // Extract the first 4 characters as org_code
        string _str = _symbol_str.substr(4, 7);

        for (auto & c: _str) {
            c = tolower(c);
        }
        return name(_str);
    }

    void notify_checks_contract(name org) {
        orgs_index _orgs( name(ORG_CONTRACT), name(ORG_CONTRACT).value );
        auto itr = _orgs.find(org.value);
        if(itr != _orgs.end()) {
        require_recipient(itr->checks_contract);
        }
    }

    name orgcode(name org, string failure_identifier) {
        orgs_index _orgs(name(ORG_CONTRACT), name(ORG_CONTRACT).value);
        auto itr = _orgs.find(org.value);
        check(itr != _orgs.end(), failure_identifier + "org not found");
        return itr->org_code;
    }
    
    struct haspackage_args {
      name org;
    };

    // Helper function to increment auto codes
    string increment_auto_code(const string& code) {
        string next_code = code;
        for (int i = code.size() - 1; i >= 0; --i) {
            if (next_code[i] == 'z') {
                next_code[i] = 'a';
            } else {
                next_code[i]++;
                break;
            }
        }
        return next_code;
    }


    void put_offchain_key_value(name org, string key, string value) {
        // Open the table in the contract's scope.
        orgs_index orgcodes(name(ORG_CONTRACT), name(ORG_CONTRACT).value);
        auto itr = orgcodes.find(org.value);
        check(itr != orgcodes.end(), "Organization does not exist");

        // Retrieve the current offchain JSON string.
        string json_str = itr->offchain_lookup_data;
        nlohmann::json j;
        if (json_str.empty()) {
            // If empty, initialize as an empty JSON object.
            j = nlohmann::json::object();
        } else if (nlohmann::json::accept(json_str)) {
            j = nlohmann::json::parse(json_str);
        } else {
            check(false, "Stored JSON is non-empty and not a valid JSON");
        }
        
        // Ensure the JSON is an object.
        check(j.is_object(), "Lookup JSON must be an object");

        // Ensure the "user" tag exists and is an object.
        if (j.find("user") == j.end() || !j["user"].is_object()) {
            j["user"] = nlohmann::json::object();
        }

        // Disallow creating a tag named "system".
        check(key != "system", "Cannot create tag 'system'");

        // Update or insert the key/value pair under the "user" object.
        j["user"][key] = value;

        // Write the updated JSON back to the table.
        orgcodes.modify(itr, same_payer, [&](auto &row) {
            row.offchain_lookup_data = j.dump();
        });
    }

    void put_onchain_key_value(name org, string key, string value) {
        // Open the table in the contract's scope.
        orgs_index orgcodes(name(ORG_CONTRACT), name(ORG_CONTRACT).value);
        auto itr = orgcodes.find(org.value);
        check(itr != orgcodes.end(), "Organization does not exist");

        // Retrieve the current onchain JSON string.
        string json_str = itr->onchain_lookup_data;
        nlohmann::json j;
        if (json_str.empty()) {
            // If empty, initialize as an empty JSON object.
            j = nlohmann::json::object();
        } else if (nlohmann::json::accept(json_str)) {
            j = nlohmann::json::parse(json_str);
        } else {
            check(false, "Stored JSON is non-empty and not a valid JSON");
        }
        
        // Ensure the JSON is an object.
        check(j.is_object(), "Lookup JSON must be an object");

        // Ensure the "user" tag exists and is an object.
        if (j.find("user") == j.end() || !j["user"].is_object()) {
            j["user"] = nlohmann::json::object();
        }

        // Disallow creating a tag named "system".
        check(key != "system", "Cannot create tag 'system'");

        // Update or insert the key/value pair under the "user" object.
        j["user"][key] = value;

        // Write the updated JSON back to the table.
        orgcodes.modify(itr, same_payer, [&](auto& row) {
            row.onchain_lookup_data = j.dump();
        });
    }




};


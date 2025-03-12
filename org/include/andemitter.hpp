#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include "orginterface.hpp"
#include "authorityinterface.hpp"

using namespace std;
using namespace eosio;
using namespace org_contract;
using namespace authority_contract;

#define SIMPLEBADGE_CONTRACT "simplebaddev"
#define BADGEDATA_CONTRACT "badgedatadev"
#define AUTHORITY_CONTRACT "authoritydev"
#define ORG_CONTRACT "organizatdev"
#define SUBSCRIPTION_CONTRACT "subscribedev"
#define BOUNTIES_CONTRACT "bountiesdevd"
#define ANDEMITTER_CONTRACT "andemittedev"

#define NEW_BADGE_ISSUANCE_NOTIFICATION BADGEDATA_CONTRACT"::notifyachiev"

CONTRACT andemitter : public contract {
public:
    using contract::contract;
    
    struct contract_asset {
        name contract;
        asset emit_asset;
    };

    [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]] void notifyachiev(
        name org,
        asset amount, 
        name from, 
        name to, 
        string memo, 
        vector<name> notify_accounts
    );

    ACTION newemission(
        name org,
        symbol emission_symbol,
        string display_name, 
        string ipfs_description,
        vector<asset> emitter_criteria,
        vector<contract_asset> emit_assets,
        bool cyclic
    );
    
    ACTION activate(name org, symbol emission_symbol);
    ACTION deactivate(name org, symbol emission_symbol);
    ACTION onckeyvalue(name org, symbol emission_symbol, string key, string value);
    ACTION offckeyvalue(name org, symbol emission_symbol, string key, string value);

private:
    
    TABLE emissions {
        symbol emission_symbol;
        string offchain_lookup_data; 
        string onchain_lookup_data; 
        map<symbol_code, asset> emitter_criteria;
        vector<contract_asset> emit_assets;
        name status; // INIT, ACTIVATE, DEACTIVATE
        bool cyclic;
        auto primary_key() const { return emission_symbol.code().raw(); }
    };
    typedef multi_index<name("emissions"), emissions> emissions_table;

    // scoped by andemitter contract
    TABLE activelookup {
        symbol badge_symbol;
        vector<symbol> active_emissions;
        auto primary_key() const { return badge_symbol.code().raw(); }
    };
    typedef multi_index<name("activelookup"), activelookup> activelookup_table;

    // scoped by account
    TABLE accounts {
        symbol emission_symbol;
        uint8_t emission_status;
        map<symbol_code, int64_t> expanded_emitter_status;
        auto primary_key() const { return emission_symbol.code().raw(); }
    };
    typedef multi_index<"accounts"_n, accounts> accounts_table;

    bool emission_exists(symbol emission_symbol, name org) {
      emissions_table emissions(name(ANDEMITTER_CONTRACT), org.value);
      return emissions.find(emission_symbol.code().raw()) != emissions.end();
    }

    enum short_emission_status : uint8_t {
        CYCLIC_IN_PROGRESS = 1,
        NON_CYCLIC_IN_PROGRESS = 2,
        NON_CYCLIC_EMITTED = 3
    };

    void invoke_action(name to, vector<contract_asset> emit_assets, uint8_t emit_factor, string emission_name, string failure_identifier) {
        for (const auto& rec : emit_assets) {
            int64_t new_amount = rec.emit_asset.amount * emit_factor;
            asset badge_asset = asset(new_amount, rec.emit_asset.symbol);
            name destination_org = get_org_from_internal_symbol(badge_asset.symbol, failure_identifier);

            if (rec.contract == name(SIMPLEBADGE_CONTRACT)) {
                action(
                    permission_level{get_self(), "active"_n},
                    name(SIMPLEBADGE_CONTRACT),
                    "issue"_n,
                    issue_args {
                        .org = destination_org,
                        .badge_asset = badge_asset,
                        .to = to,
                        .memo = emission_name }
                ).send();
            } else if (rec.contract == name(BOUNTIES_CONTRACT)) {
                action(
                    permission_level{get_self(), "active"_n},
                    name(BOUNTIES_CONTRACT),
                    "distribute"_n,
                    issue_args {
                        .org = destination_org,
                        .badge_asset = badge_asset,
                        .to = to,
                        .memo = emission_name }
                ).send();        
            } else {
            
            }


            // Extend with more conditions if other contracts are to be called
        }
    }    

    void update_expanded_emitter_status(name account, map<symbol_code, int64_t>& expanded_emitter_status, const asset& new_asset, emissions emission, uint8_t& emission_status, string failure_identifier) {
        expanded_emitter_status[new_asset.symbol.code()] += new_asset.amount;

        bool criteria_met = true;
        int min_multiplier = INT_MAX;
        for (const auto& [symbol, crit_asset] : emission.emitter_criteria) {
            auto it = expanded_emitter_status.find(symbol);
            if (it == expanded_emitter_status.end() || it->second < crit_asset.amount) {
                criteria_met = false;
                break;
            } else {
                int multiplier = it->second / crit_asset.amount;
                min_multiplier = std::min(min_multiplier, multiplier);
            }
        }

        if (criteria_met) {
            if (!emission.cyclic && min_multiplier > 1) {
                min_multiplier = 1; // Adjust for non-cyclic emissions
            }

            if (!emission.cyclic) {
                emission_status = (min_multiplier > 0) ? NON_CYCLIC_EMITTED : NON_CYCLIC_IN_PROGRESS;
            } else {
                emission_status = CYCLIC_IN_PROGRESS;
            }

            if (min_multiplier > 0) {
                for (auto& [symbol, amount] : expanded_emitter_status) {
                    amount -= emission.emitter_criteria[symbol].amount * min_multiplier;
                    if (amount < 0) amount = 0; // Prevent negative values
                }
                invoke_action(account, emission.emit_assets, min_multiplier, emission.emission_symbol.code().to_string(), failure_identifier); // Take action based on the new status
            }
        }
    }
    struct billing_args {
      name org;
      uint8_t actions_used;
    };
    struct issue_args {
      name org;
      asset badge_asset;
      name to;
      string memo;
    };

};

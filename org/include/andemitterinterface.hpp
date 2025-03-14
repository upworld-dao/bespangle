#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>

using namespace eosio;
using namespace std;

#define ANDEMITTER_CONTRACT "andemittedev"

namespace andemitter_contract {

    struct contract_asset {
        name contract;
        asset emit_asset;
    };
    
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


}

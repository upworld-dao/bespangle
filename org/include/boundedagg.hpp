#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>
#include <eosio/crypto.hpp>
#include <eosio/system.hpp>
#include "orginterface.hpp"
#include "authorityinterface.hpp"

using namespace eosio;
using namespace std;
using namespace org_contract;
using namespace authority_contract;

#define AUTHORITY_CONTRACT "authoritydev"
#define ORG_CONTRACT "organizatdev"
#define BADGEDATA_CONTRACT "badgedatadev"
#define SUBSCRIPTION_CONTRACT "subscribedev"
#define BOUNDED_AGG_CONTRACT "boundedagdev"

#define NEW_BADGE_ISSUANCE_NOTIFICATION BADGEDATA_CONTRACT"::notifyachiev"

CONTRACT boundedagg : public contract {
public:
    using contract::contract;

    [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]] void notifyachiev(
    name org,
    asset badge_asset, 
    name from, 
    name to, 
    string memo, 
    vector<name> notify_accounts);


    ACTION initagg(name org, symbol agg_symbol, vector<symbol> init_badge_symbols, string ipfs_image, string display_name, string description);
    ACTION addinitbadge(name org, symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION reminitbadge(name org, symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION initseq(name org, symbol agg_symbol, string sequence_description);
    ACTION actseq(name org, symbol agg_symbol, vector<uint64_t> seq_ids);
    ACTION actseqai(name org, symbol agg_symbol);
    ACTION actseqfi(name org, symbol agg_symbol);
    ACTION endseq(name org, symbol agg_symbol, vector<uint64_t> seq_ids);
    ACTION endseqaa(name org, symbol agg_symbol);
    ACTION endseqfa(name org, symbol agg_symbol);
    ACTION addbadge(name org, symbol agg_symbol, vector<uint64_t> seq_ids, vector<symbol> badge_symbols);
    ACTION addbadgefa(name org, symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION addbadgeaa(name org, symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION addbadgefi(name org, symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION addbadgeli(name org, symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION addbadgeai(name org, symbol agg_symbol, vector<symbol> badge_symbols);

    ACTION pauseall(name org, symbol agg_symbol, uint64_t seq_id);
    ACTION pausebadge(name org, symbol agg_symbol, uint64_t badge_agg_seq_id);
    ACTION pausebadges(name org, symbol agg_symbol, uint64_t seq_id, vector<symbol> badge_symbols);
    ACTION pauseallfa(name org, symbol agg_symbol);
    ACTION resumeall(name org, symbol agg_symbol, uint64_t seq_id);
    ACTION resumebadge(name org, symbol agg_symbol, uint64_t badge_agg_seq_id);
    ACTION resumebadges(name org, symbol agg_symbol, uint64_t seq_id, vector<symbol> badge_symbols);

    ACTION offckeyvalue(name org, 
        symbol agg_symbol, 
        string key, 
        string value);

    ACTION onckeyvalue(name org, 
        symbol agg_symbol, 
        string key, 
        string value);
    
private:

    struct [[eosio::table]] badgestatus {
        uint64_t badge_agg_seq_id; // Primary key: Unique ID for each badge-sequence association
        symbol agg_symbol;         // The aggregation symbol associated with this badge
        uint64_t seq_id;           // The sequence ID this badge is associated with
        symbol badge_symbol;       // The symbol representing the badge
        name badge_status;         // The status of the badge (e.g., "active")
        name seq_status;           // The status of the sequence copied from the sequence table

        uint64_t primary_key() const { return badge_agg_seq_id; }
        uint128_t by_agg_seq() const { return combine_keys(agg_symbol.code().raw(), seq_id); }

        checksum256 by_badge_status() const {
            // Example hashing for badge status; adjust according to actual implementation needs
            auto data = badge_symbol.code().to_string() + badge_status.to_string() + seq_status.to_string();
            return sha256(data.data(), data.size());
        }

        checksum256 by_agg_seq_badge() const {
            // Example hashing for badge status; adjust according to actual implementation needs
            auto data = agg_symbol.code().to_string() + std::to_string(seq_id) + badge_symbol.code().to_string();
            return sha256(data.data(), data.size());
        }

        static uint128_t combine_keys(uint64_t a, uint64_t b) {
            // Combines two 64-bit integers into a single 128-bit value for indexing purposes
            return (static_cast<uint128_t>(a) << 64) | b;
        }
    };
    typedef eosio::multi_index<"badgestatus"_n, badgestatus,
        eosio::indexed_by<"byaggseq"_n, eosio::const_mem_fun<badgestatus, uint128_t, &badgestatus::by_agg_seq>>,
        eosio::indexed_by<"bybadgestat"_n, eosio::const_mem_fun<badgestatus, checksum256, &badgestatus::by_badge_status>>,
        eosio::indexed_by<"aggseqbadge"_n, eosio::const_mem_fun<badgestatus, checksum256, &badgestatus::by_agg_seq_badge>>
    > badgestatus_table;

    checksum256 hash_active_status(const symbol& badge_symbol, const name& badge_status, const name& seq_status) {
        // Create a data string from badge_symbol, badge_status, and seq_status
        string data_str = badge_symbol.code().to_string() + badge_status.to_string() + seq_status.to_string();

        // Return the sha256 hash of the concatenated string
        return sha256(data_str.data(), data_str.size());
    }

    checksum256 hash_agg_seq_badge(const symbol& agg_symbol, uint64_t seq_id, symbol badge_symbol) {
        // Create a data string from badge_symbol, badge_status, and seq_status
        string data_str = agg_symbol.code().to_string() + std::to_string(seq_id) + badge_symbol.code().to_string();

        // Return the sha256 hash of the concatenated string
        return sha256(data_str.data(), data_str.size());
    }

    // scoped by account
    struct [[eosio::table]] achievements {
        uint64_t badge_agg_seq_id;
        uint64_t count;

        uint64_t primary_key() const { return badge_agg_seq_id; }
    };
    typedef eosio::multi_index<"achievements"_n, achievements> achievements_table;

    // scoped by agg symbol
    struct [[eosio::table]] sequence {
        uint64_t seq_id;
        name seq_status; // init, end, active
        string sequence_description;
        time_point_sec init_time;
        time_point_sec active_time;
        time_point_sec end_time;

        uint64_t primary_key() const { return seq_id; }
    };
    typedef eosio::multi_index<"sequence"_n, sequence> sequence_table;

    // scoped by org
    struct [[eosio::table]] aggdetail {
        symbol agg_symbol;
        string offchain_lookup_data;
        string onchain_lookup_data;
        uint64_t last_init_seq_id;
        vector<uint64_t> init_seq_ids;
        vector<uint64_t> active_seq_ids;
        vector<uint64_t> end_seq_ids;
        vector<symbol> init_badge_symbols;
        uint64_t primary_key() const { return agg_symbol.code().raw(); }
    };
    typedef eosio::multi_index<"aggdetails"_n, aggdetail> aggdetail_table;



    // scoped by boundedagg contract
    struct [[eosio::table]] aggseqinfo {
        uint64_t last_badge_agg_seq_id;
        uint64_t primary_key() const { return last_badge_agg_seq_id; }
    };

    typedef eosio::multi_index<"aggseqinfo"_n, aggseqinfo> aggseqinfo_tbl;


    void update_status_in_badgestatus(name org, const symbol& agg_symbol, uint64_t seq_id, const name& new_status, string failure_identifier) {
        badgestatus_table badgestatus_t(get_self(), org.value);
        auto index = badgestatus_t.get_index<"byaggseq"_n>();
        auto key = badgestatus::combine_keys(agg_symbol.code().raw(), seq_id);
        auto itr = index.find(key);
        while(itr != index.end() && itr->agg_symbol == agg_symbol && itr->seq_id == seq_id) {
            index.modify(itr, _self, [&](auto& row) {
                row.seq_status = new_status;
            });
            itr++;
        }
    }

    void insert_record_in_badgestatus(name org, const symbol& agg_symbol, uint64_t seq_id, const symbol& badge_symbol, const name& seq_status, string failure_identifier) {
        badgestatus_table badgestatus_t(get_self(), org.value);
        auto index = badgestatus_t.get_index<"aggseqbadge"_n>();
        auto key = hash_agg_seq_badge(agg_symbol, seq_id, badge_symbol);
        auto itr = index.find(key);
        if (itr != index.end() && itr->agg_symbol == agg_symbol && itr->seq_id == seq_id && itr->badge_symbol == badge_symbol) {
            return;
        }

        aggseqinfo_tbl a_tbl(get_self(), get_self().value);
        auto aggseqinfo = a_tbl.begin();
        uint64_t last_badge_agg_seq_id = 1;
        if(aggseqinfo == a_tbl.end()) {
            
            a_tbl.emplace(get_self(), [&](auto& row) {
                row.last_badge_agg_seq_id = 1;
            }); 
            
        } else {
            last_badge_agg_seq_id = aggseqinfo->last_badge_agg_seq_id + 1;
            a_tbl.emplace(_self, [&](auto& row) {
                row.last_badge_agg_seq_id = last_badge_agg_seq_id;
            });
            a_tbl.erase(aggseqinfo);
            
        }
        
        
        badgestatus_t.emplace(get_self(), [&](auto& row) {
            row.badge_agg_seq_id = last_badge_agg_seq_id;
            row.agg_symbol = agg_symbol;
            row.seq_id = seq_id;
            row.badge_symbol = badge_symbol;
            row.badge_status = "active"_n;
            row.seq_status = seq_status;
        });

    }

    vector<uint64_t> all_init_seq_ids (name org, symbol agg_symbol, string failure_identifier) {
        aggdetail_table aggdetail_t(_self, org.value);
        auto aggdetail_itr = aggdetail_t.find(agg_symbol.code().raw());

        check(aggdetail_itr != aggdetail_t.end(), failure_identifier + "Aggregation symbol does not exist.");
        vector<uint64_t> init_seq_ids = aggdetail_itr->init_seq_ids;
        check(!init_seq_ids.empty(), failure_identifier + "No seq id in init state");
        return init_seq_ids;
    }

    vector<uint64_t> all_active_seq_ids (name org, symbol agg_symbol, string failure_identifier) {
        aggdetail_table aggdetail_t(_self, org.value);
        auto aggdetail_itr = aggdetail_t.find(agg_symbol.code().raw());

        check(aggdetail_itr != aggdetail_t.end(), failure_identifier + "Aggregation symbol does not exist.");
        vector<uint64_t> active_seq_ids = aggdetail_itr->active_seq_ids;
        check(!active_seq_ids.empty(), failure_identifier + "No seq id in active state");
        return active_seq_ids;
    }

    void update_badge_status_all(name org, symbol agg_symbol, uint64_t seq_id, name status, string failure_identifier) {
        badgestatus_table badgestatus_t(get_self(), org.value);
        auto index = badgestatus_t.get_index<"byaggseq"_n>();
        auto key = badgestatus::combine_keys(agg_symbol.code().raw(), seq_id);
        auto itr = index.find(key);
        while(itr != index.end() && itr->agg_symbol == agg_symbol && itr->seq_id == seq_id) {
            index.modify(itr, _self, [&](auto& row) {
                row.badge_status = status;
            });
            itr++;
        }
    }

    void update_badge_status(name org, symbol agg_symbol, uint64_t badge_agg_seq_id, name status, string failure_identifier) {
        badgestatus_table badgestatus_t(get_self(), org.value);
        auto itr = badgestatus_t.find(badge_agg_seq_id);
        check(itr != badgestatus_t.end(), failure_identifier + "invalid badge_seq_id");
        check(itr->agg_symbol == agg_symbol, failure_identifier + "agg_symbol mismatch");
        badgestatus_t.modify(itr, _self, [&](auto& row) {
            row.badge_status = status;
        });
    }

    void update_badge_statuses(name org, symbol agg_symbol, uint64_t seq_id, vector<symbol> badge_symbols, name status, string failure_identifier) {
        badgestatus_table badgestatus_t(get_self(), org.value);
        auto index = badgestatus_t.get_index<"aggseqbadge"_n>();
        for(auto i = 0; i < badge_symbols.size(); i++) {
            auto key = hash_agg_seq_badge(agg_symbol, seq_id, badge_symbols[i]);
            auto itr = index.find(key);
            if(itr != index.end() && 
                itr->agg_symbol == agg_symbol && 
                itr->seq_id == seq_id && 
                itr->badge_symbol == badge_symbols[i]) {

                index.modify(itr, _self, [&](auto& row) {
                    row.badge_status = status;
                });

            }
        }
    }

    struct actseq_args {
        name org;
        symbol agg_symbol;
        vector<uint64_t> seq_ids;
    };

    struct endseq_args {
        name org;
        symbol agg_symbol;
        vector<uint64_t> seq_ids;
    };

    struct addbadge_args {
        name org;
        symbol agg_symbol;
        vector<uint64_t> seq_ids;
        vector<symbol> badge_symbols;
    };

    struct addbadgeli_args {
        name org;
        symbol agg_symbol;
        vector<symbol> badge_symbols;
    };

    struct pauseall_args {
        name org;
        symbol agg_symbol;
        uint64_t seq_id;
    };


    struct billing_args {
      name org;
      uint8_t actions_used;
    };

};

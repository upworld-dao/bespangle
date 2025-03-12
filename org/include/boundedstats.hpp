#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include "authorityinterface.hpp"
#include "boundedagginterface.hpp"

using namespace std;
using namespace eosio;
using namespace authority_contract;
using namespace boundedagg_contract;

#define BADGEDATA_CONTRACT "badgedatadev"
#define BOUNDED_AGG_CONTRACT "boundedagdev"
#define AUTHORITY_CONTRACT "authoritydev"
#define ORG_CONTRACT "organizatdev"
#define SUBSCRIPTION_CONTRACT "subscribedev"

#define NEW_BADGE_ISSUANCE_NOTIFICATION BADGEDATA_CONTRACT"::notifyachiev"

CONTRACT boundedstats : public contract {
  public:
    using contract::contract;

    [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]] void notifyachiev(
        name org,
        asset badge_asset, 
        name from, 
        name to, 
        string memo, 
        vector<name> notify_accounts);


    ACTION activate(name org, symbol agg_symbol, vector<symbol> badge_symbols);

    ACTION deactivate(name org, symbol agg_symbol, vector<symbol> badge_symbols);

  private:
    //scoped by agg_symbol
    TABLE  statssetting {
        symbol badge_symbol;
        uint64_t primary_key() const { return badge_symbol.code().raw(); }
    };
    typedef multi_index<"statssetting"_n, statssetting> statssetting_table;

    // scoped by org
    TABLE counts {
        uint64_t badge_agg_seq_id;
        uint64_t total_recipients;
        uint64_t total_issued;
        uint64_t primary_key() const { return badge_agg_seq_id; }
    };
    typedef multi_index<"counts"_n, counts> counts_table;

    // scoped by badge_agg_seq_id
    TABLE ranks {
        vector<name> accounts;
        uint64_t balance;
        uint64_t primary_key() const { return balance; }
    };
    typedef multi_index<"ranks"_n, ranks> ranks_table;

    void update_rank(name org, name account, uint64_t badge_agg_seq_id, uint64_t old_balance, uint64_t new_balance) {

        ranks_table _ranks(get_self(), badge_agg_seq_id); // Use badge_agg_seq_id as scope
        if (old_balance != new_balance && old_balance != 0) {
            auto old_itr = _ranks.find(old_balance);
            check(old_itr != _ranks.end(),"not found in boundedstats");
            auto old_names = old_itr->accounts;
            old_names.erase(std::remove(old_names.begin(), old_names.end(), account), old_names.end());
            if (old_names.empty()) {
                _ranks.erase(old_itr); // Remove the score entry if no names are left
            } else {
                _ranks.modify(old_itr, get_self(), [&](auto& entry) {
                    entry.accounts = old_names;
                });
            }
        }
        auto new_itr = _ranks.find(new_balance);
        if (new_itr == _ranks.end()) {
            _ranks.emplace(get_self(), [&](auto& entry) {
                entry.balance = new_balance;
                entry.accounts.push_back(account);
            });
        } else {
            if (std::find(new_itr->accounts.begin(), new_itr->accounts.end(), account) == new_itr->accounts.end()) {
                _ranks.modify(new_itr, get_self(), [&](auto& entry) {
                    entry.accounts.push_back(account);
                });
            } // If player name already exists for this score, no action needed
        }

    }

    void update_count(name org, name account, uint64_t badge_agg_seq_id, uint64_t old_balance, uint64_t new_balance) {
        counts_table _counts(get_self(), org.value);
        auto counts_itr = _counts.find(badge_agg_seq_id);
        uint64_t total_recipients;
        uint64_t total_issued;
        if(counts_itr == _counts.end()) {
            total_recipients = 1;
            total_issued = new_balance;
            _counts.emplace(get_self(), [&](auto& entry) {
                entry.badge_agg_seq_id = badge_agg_seq_id;
                entry.total_recipients = total_recipients;
                entry.total_issued = total_issued;
            });
        } else {
            total_recipients = counts_itr->total_recipients;
            if(old_balance == 0) {
                total_recipients++;
            }
            total_issued = counts_itr->total_issued + new_balance - old_balance;
            _counts.modify(counts_itr, get_self(), [&](auto& entry) {
                entry.total_recipients = total_recipients;
                entry.total_issued = total_issued;
            });
        }
    }

    // Function to fetch the new balance from the achievements table, scoped by account.
    uint64_t get_new_balance(name account, uint64_t badge_agg_seq_id) {
        // Access the achievements table with the account as the scope.
        achievements_table achievements(name(BOUNDED_AGG_CONTRACT), account.value);
        auto achv_itr = achievements.find(badge_agg_seq_id);
        
        // Check if the record exists and return the balance.
        if (achv_itr != achievements.end()) {
            return achv_itr->count; // Assuming 'balance' is the field storing the balance.
        } else {
            // Handle the case where there is no such achievement record.
            eosio::check(false, "Achievement record not found for the given badge_agg_seq_id.");
            return 0; // This return is formal as the check above will halt execution if triggered.
        }
    }
    


    struct billing_args {
      name org;
      uint8_t actions_used;
    };

};

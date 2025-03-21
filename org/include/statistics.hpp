#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include "cumulativeinterface.hpp"
#include "badgedatainterface.hpp"

#define CUMULATIVE_CONTRACT "cumulativdev"
#define BADGEDATA_CONTRACT "badgedatadev"
#define AUTHORITY_CONTRACT "authoritydev"
#define ORG_CONTRACT "organizatdev"
#define SUBSCRIPTION_CONTRACT "subscribedev"

#define NEW_BADGE_ISSUANCE_NOTIFICATION BADGEDATA_CONTRACT"::notifyachiev"


using namespace std;
using namespace eosio;
using namespace cumulative_contract;
using namespace badgedata_contract;

CONTRACT statistics : public contract {
  public:
    using contract::contract;

    ACTION dummy();

    [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]] void notifyachiev(
    name org,
    asset badge_asset, 
    name from, 
    name to, 
    string memo, 
    vector<name> notify_accounts);

  private:
    // scoped by org
    TABLE counts {
        symbol badge_symbol;
        uint64_t total_recipients;
        uint64_t total_issued;
        uint64_t primary_key() const { return badge_symbol.code().raw(); }
    };
    typedef multi_index<"counts"_n, counts> counts_table;

    // scoped by badge_symbol
    TABLE ranks {
        vector<name> accounts;
        uint64_t balance;
        uint64_t primary_key() const { return balance; }
    };
    typedef multi_index<"ranks"_n, ranks> ranks_table;


    void update_rank(name org, name account, symbol badge_symbol, uint64_t old_balance, uint64_t new_balance) {

        ranks_table _ranks(get_self(), badge_symbol.code().raw()); // Use badge_agg_seq_id as scope
        // Check and remove the player's name from the old score, if provided and different

        if (old_balance != new_balance) {
            auto old_itr = _ranks.find(old_balance);
            if (old_itr != _ranks.end()) {
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

    void update_count(name org, name account, symbol badge_symbol, uint64_t old_balance, uint64_t new_balance) {
        counts_table _counts(get_self(), org.value);
        auto counts_itr = _counts.find(badge_symbol.code().raw());
        uint64_t total_recipients;
        uint64_t total_issued;
        if(counts_itr == _counts.end()) {
            total_recipients = 1;
            total_issued = new_balance;
            _counts.emplace(get_self(), [&](auto& entry) {
                entry.badge_symbol = badge_symbol;
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

    struct billing_args {
      name org;
      uint8_t actions_used;
    };

};

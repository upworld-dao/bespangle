#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include "orginterface.hpp"

using namespace std;
using namespace eosio;
using namespace org_contract;

#define SIMPLEBADGE_CONTRACT "simplebaddev"
#define ORG_CONTRACT "organizatdev"
#define BADGEDATA_CONTRACT "badgedatadev"
#define STATISTICS_CONTRACT "statisticdev"

#define CUMULATIVE_CONTRACT "cumulativdev"
#define SUBSCRIPTION_CONTRACT "subscribedev"


CONTRACT simmanager : public contract {
  public:
    using contract::contract;

    ACTION initsimple (name authorized, 
      symbol badge_symbol,
      string display_name,
      string ipfs_image,
      string description,
      bool lifetime_aggregate,
      bool lifetime_stats,
      string memo);
      
    ACTION givesimple (name authorized,
     symbol badge_symbol,
     uint64_t amount,
     name to, 
     string memo );

    ACTION simplebatch (name authorized,
      symbol badge_symbol,
      uint64_t amount,
      vector<name> to, 
      string memo);

    ACTION addbadgeauth (name org, name action, name badge, name authorized_account);

    ACTION delbadgeauth (name org, name action, name badge, name authorized_account);

    ACTION addactionauth (name org, name action, name authorized_account);

    ACTION delactionauth (name org, name action, name authorized_account);

  private:
    TABLE actionauths {
      name  action;
      vector<name>  authorized_accounts;
      auto primary_key() const { return action.value; }
    };
    typedef multi_index<name("actionauths"), actionauths> actionauths_table;

    TABLE badgeauths {
      uint64_t id;
      name action;
      name badge;
      vector<name> authorized_accounts;

      uint64_t primary_key() const { return id; }
      uint128_t secondary_key() const { return combine_names(action, badge); }

      static uint128_t combine_names(const name& a, const name& b) {
          return (uint128_t{a.value} << 64) | b.value;
      }
    };

    typedef eosio::multi_index<"badgeauths"_n, badgeauths,
        indexed_by<"byactionbadge"_n, const_mem_fun<badgeauths, uint128_t, &badgeauths::secondary_key>>
    > badgeauths_table;

    bool has_action_authority (name org, name action, name account) {
      actionauths_table _actionauths_table (get_self(), org.value);
      auto itr = _actionauths_table.find(action.value);
      if(itr == _actionauths_table.end()) {
        return false;
      }
      auto authorized_accounts = itr->authorized_accounts;
      for(auto i = 0; i < authorized_accounts.size(); i++) {
        if(authorized_accounts[i] == account) {
          return true;
        }
      }
      return false;
    }

    bool has_badge_authority (name org, name action, name badge, name account) {
      badgeauths_table badgeauths(get_self(), org.value);
      auto secondary_index = badgeauths.get_index<"byactionbadge"_n>();
      uint128_t secondary_key = badgeauths::combine_names(action, badge);
      auto itr = secondary_index.find(secondary_key);
      if(itr == secondary_index.end() || itr->action != action || itr->badge != badge) {
        return false;
      }
      auto authorized_accounts = itr->authorized_accounts;
      for(auto i = 0; i < authorized_accounts.size(); i++) {
        if(authorized_accounts[i] == account) {
          return true;
        }
      }
      return false;
    }    
    struct addfeature_args {
      name org;
      symbol badge_symbol;
      name notify_account;
      string memo;
    };

    struct createsimple_args {
      name org;
      symbol badge_symbol;
      string display_name;
      string ipfs_image;
      string description;
      string memo;
    };

    struct issuesimple_args {
      name org;
      asset badge_asset;
      name to;
      string memo;
    };

};

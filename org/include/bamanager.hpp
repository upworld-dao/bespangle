#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include "orginterface.hpp"
#include "authorityinterface.hpp"

using namespace std;
using namespace eosio;
using namespace org_contract;
using namespace authority_contract;

#define BOUNDED_AGG_CONTRACT "boundedagdev"
#define BADGEDATA_CONTRACT "badgedatadev"
#define ORG_CONTRACT "organizatdev"
#define BOUNDED_STATS_CONTRACT "boundedstdev"
/* #undef BOUNDED_AGG_VALIDATION_CONTRACT */

CONTRACT bamanager : public contract {
  public:
    using contract::contract;

    ACTION initagg(name authorized, symbol agg_symbol, vector<symbol> badge_symbols, vector<symbol> stats_badge_symbols, string ipfs_image, string display_name, string description);
    ACTION addinitbadge(name authorized, symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION reminitbadge(name authorized, symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION addstatbadge(name authorized, symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION remstatbadge(name authorized, symbol agg_symbol, vector<symbol> badge_symbols);


    ACTION initseq(name authorized, symbol agg_symbol, vector<symbol> badge_symbols, string sequence_description);
    ACTION actseq(name authorized, symbol agg_symbol, vector<uint64_t> seq_ids);


    ACTION actseqai(name authorized, symbol agg_symbol);
    ACTION actseqfi(name authorized, symbol agg_symbol);
    ACTION endseq(name authorized, symbol agg_symbol, vector<uint64_t> seq_ids);
    ACTION endseqaa(name authorized, symbol agg_symbol);
    ACTION endseqfa(name authorized, symbol agg_symbol);
    ACTION addbadge(name authorized, symbol agg_symbol, vector<uint64_t> seq_ids, vector<symbol> badge_symbols);

    ACTION pauseall(name authorized, symbol agg_symbol, uint64_t seq_id);
    ACTION pausebadge(name authorized, symbol agg_symbol, uint64_t badge_agg_seq_id);
    ACTION pausebadges(name authorized, symbol agg_symbol, uint64_t seq_id, vector<symbol> badge_symbols);
    ACTION pauseallfa(name authorized, symbol agg_symbol);
    ACTION resumeall(name authorized, symbol agg_symbol, uint64_t seq_id);
    ACTION resumebadge(name authorized, symbol agg_symbol, uint64_t badge_agg_seq_id);
    ACTION resumebadges(name authorized, symbol agg_symbol, uint64_t seq_id, vector<symbol> badge_symbols);

    ACTION addaggauth(name org, name action, name agg, name authorized_account);
    ACTION delaggauth(name org, name action, name agg, name authorized_account);
    ACTION addactionauth (name org, name action, name authorized_account);
    ACTION delactionauth (name org, name action, name authorized_account);
  
  private:

    TABLE actionauths {
      name  action;
      vector<name>  authorized_accounts;
      auto primary_key() const { return action.value; }
    };
    typedef multi_index<name("actionauths"), actionauths> actionauths_table;

    TABLE aggauths {
      uint64_t id;
      name action;
      name agg;
      vector<name> authorized_accounts;

      uint64_t primary_key() const { return id; }
      uint128_t secondary_key() const { return combine_names(action, agg); }

      static uint128_t combine_names(const name& a, const name& b) {
          return (uint128_t{a.value} << 64) | b.value;
      }
    };

    typedef eosio::multi_index<"aggauths"_n, aggauths,
        indexed_by<"byactionagg"_n, const_mem_fun<aggauths, uint128_t, &aggauths::secondary_key>>
    > aggauths_table;

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

    bool has_agg_authority (name org, name action, name agg, name account) {
      aggauths_table aggauths(get_self(), org.value);
      auto secondary_index = aggauths.get_index<"byactionagg"_n>();
      uint128_t secondary_key = aggauths::combine_names(action, agg);
      auto itr = secondary_index.find(secondary_key);
      if(itr == secondary_index.end() || itr->action != action || itr->agg != agg) {
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
    
    struct initagg_args {
      name org;
      symbol agg_symbol;
      vector<symbol> init_badge_symbols;
      string ipfs_image;
      string display_name; 
      string description;
    };

    struct stats_activate_args {
      name org;
      symbol agg_symbol;
      vector<symbol> badge_symbols;
    };

    struct stats_deactivate_args {
      name org;
      symbol agg_symbol;
      vector<symbol> badge_symbols;
    };

    struct addinitbadge_args {
      name org;
      symbol agg_symbol;
      vector<symbol> badge_symbols;
    };

    struct reminitbadge_args {
      name org;
      symbol agg_symbol;
      vector<symbol> badge_symbols;
    };


    struct initseq_args {
      name org;
      symbol agg_symbol;
      string sequence_description;
    };

    struct actseq_args {
      name org;
      symbol agg_symbol;
      vector<uint64_t> seq_ids;
    };

    struct actseqai_args {
      name org;
      symbol agg_symbol;
    };

    struct actseqfi_args {
      name org;
      symbol agg_symbol;
    };

    struct endseq_args {
      name org;
      symbol agg_symbol;
      vector<uint64_t> seq_ids;
    };
 
    struct endseqaa_args {
      name org;
      symbol agg_symbol;
    };

    struct endseqfa_args {
      name org;
      symbol agg_symbol;
    };

    struct addbadge_args {
      name org;
      symbol agg_symbol;
      vector<uint64_t> seq_ids;
      vector<symbol> badge_symbols;
    };

    struct addbadgefa_args {
      name org;
      symbol agg_symbol;
      vector<symbol> badge_symbols;
    };

    struct addbadgeaa_args {
      name org;
      symbol agg_symbol;
      vector<symbol> badge_symbols;
    };

    struct addbadgefi_args {
      name org;
      symbol agg_symbol;
      vector<symbol> badge_symbols;
    };  

    struct addbadgeai_args {
      name org;
      symbol agg_symbol;
      vector<symbol> badge_symbols;
    };

    struct addbadgeli_args {
      name org;
      symbol agg_symbol;
      vector<symbol> badge_symbols;
    };

    struct addfeature_args {
      name org;
      symbol badge_symbol;
      name notify_account;
      string memo;
    };

    struct pauseall_args {
      name org;
      symbol agg_symbol;
      uint64_t seq_id;
    };

    struct pausebadge_args {
      name org;
      symbol agg_symbol;
      uint64_t badge_agg_seq_id;
    };

    struct pausebadges_args {
      name org;
      symbol agg_symbol;
      uint64_t seq_id;
      vector<symbol> badge_symbols;
    };

    struct pauseallfa_args {
      name org;
      symbol agg_symbol;
    };

    struct resumeall_args {
      name org;
      symbol agg_symbol;
      uint64_t seq_id;
    };

    struct resumebadge_args {
      name org;
      symbol agg_symbol;
      uint64_t badge_agg_seq_id;
    };

    struct resumebadges_args {
      name org;
      symbol agg_symbol;
      uint64_t seq_id;
      vector<symbol> badge_symbols;
    };

};



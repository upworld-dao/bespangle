#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

#include "authorityinterface.hpp"

using namespace std;
using namespace eosio;
using namespace authority_contract;

#define BADGEDATA_CONTRACT "badgedatadev"

CONTRACT badgedata : public contract {
public:
  using contract::contract;

  ACTION initbadge(
    name org,
    symbol badge_symbol, 
    string display_name, 
    string ipfs_image,
    string description, 
    string memo);

  ACTION addfeature(
    name org,
    symbol badge_symbol, 
    name notify_account, 
    string memo);

  ACTION addnotify(
    name org,
    symbol badge_symbol, 
    name notify_account, 
    string memo, 
    string offchain_lookup_data, 
    string onchain_lookup_data, 
    uint64_t rarity_counts);

  ACTION delfeature(
    name org,
    symbol badge_symbol, 
    name notify_account, 
    string memo);

  ACTION delnotify(
    name org,
    symbol badge_symbol, 
    name notify_account, 
    string memo, 
    string offchain_lookup_data, 
    string onchain_lookup_data, 
    uint64_t rarity_counts);

  ACTION achievement(
    name org,
    asset badge_asset, 
    name from, 
    name to, 
    string memo);

  ACTION notifyachiev(
    name org,
    asset badge_asset, 
    name from, 
    name to, 
    string memo, 
    vector<name> notify_accounts);

  ACTION offckeyvalue(name org, 
    symbol badge_symbol, 
    string key, 
    string value);

  ACTION onckeyvalue(name org, 
    symbol badge_symbol, 
    string key, 
    string value);

private:

    
  struct ramcredits_arg {
    name org;
    name contract;
    uint64_t bytes;
    string memo;
  };

  struct syscredits_arg {
    name org;
  };

  struct notifyachievement_args {
    name org;
    asset badge_asset;
    name from;
    name to;
    string memo;
    vector<name> notify_accounts;
  };

  struct downstream_notify_args {
    name org;
    symbol badge_symbol;
    name notify_account;
    string memo;
    string offchain_lookup_data;
    string onchain_lookup_data;
    uint64_t rarity_counts;
  };

  struct checkallow_args {
    name org;
    name account;
  };

  struct local_addfeature_args {
    name org;
    name badge;
    name notify_account;
    string memo;
  };

  TABLE badge {
    symbol badge_symbol;
    vector<name> notify_accounts;
    string offchain_lookup_data;
    string onchain_lookup_data;
    uint64_t rarity_counts;
    auto primary_key() const { return badge_symbol.code().raw(); }
  };
  typedef multi_index<name("badge"), badge> badge_table;

  bool badge_exists(symbol badge_symbol) {
      badge_table badges(name(BADGEDATA_CONTRACT), name(BADGEDATA_CONTRACT).value);
      return badges.find(badge_symbol.code().raw()) != badges.end();
  }
};


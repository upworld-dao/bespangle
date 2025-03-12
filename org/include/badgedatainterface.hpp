#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>

using namespace eosio;
using namespace std;

#define BADGEDATA_CONTRACT "badgedatadev"

namespace badgedata_contract {
    
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

  bool badge_exists(symbol badge_symbol, name org) {
      badge_table badges(name(BADGEDATA_CONTRACT), org.value);
      return badges.find(badge_symbol.code().raw()) != badges.end();
  }
}

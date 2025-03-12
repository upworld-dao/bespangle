#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include "authorityinterface.hpp"

using namespace std;
using namespace eosio;
using namespace authority_contract;

#define BADGEDATA_CONTRACT "badgedatadev"
#define SUBSCRIPTION_CONTRACT "subscribedev"

CONTRACT simplebadge : public contract {
  public:
    using contract::contract;

    ACTION create (
      name org,
      symbol badge_symbol, 
      string display_name, 
      string ipfs_image,
      string description, 
      string memo);

    ACTION issue (name org, asset badge_asset, name to, string memo );


  private:

    struct achievement_args {
      name org;
      asset badge_asset;
      name from;
      name to;
      string memo;
    };

    struct initbadge_args {
      name org;
      symbol badge_symbol;
      string display_name; 
      string ipfs_image;
      string description;
      string memo;
    };

    struct billing_args {
      name org;
      uint8_t actions_used;
    };

};

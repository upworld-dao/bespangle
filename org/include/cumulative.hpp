#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

#define BADGEDATA_CONTRACT "badgedatadev"
#define SUBSCRIPTION_CONTRACT "subscribedev"
#define CUMULATIVE_CONTRACT "cumulativdev"


#define NEW_BADGE_ISSUANCE_NOTIFICATION BADGEDATA_CONTRACT"::notifyachiev"


CONTRACT cumulative : public contract {
public:
  using contract::contract;
  
  [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]] void notifyachiev(
    name org,
    asset badge_asset, 
    name from, 
    name to, 
    string memo, 
    vector<name> notify_accounts);

  ACTION dummy();

private:
  TABLE account {
      asset    balance;
      uint64_t primary_key() const { return balance.symbol.code().raw(); }
  };
  typedef eosio::multi_index<"accounts"_n, account> accounts;

  struct billing_args {
    name org;
    uint8_t actions_used;
  };

};


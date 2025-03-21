#include <eosio/eosio.hpp>

using namespace std;
using namespace eosio;

#define AUTHORITY_CONTRACT "authoritydev"

CONTRACT authority : public contract {
  public:
    using contract::contract;

    ACTION addauth(name contract, name action, name authorized_contract);
    ACTION removeauth(name contract, name action, name authorized_contract);
    ACTION hasauth(name contract, name action, name account);

  private:
    TABLE auth {
        name action;
        vector<name> authorized_contracts;
        uint64_t primary_key() const { return action.value; }
    };
    typedef eosio::multi_index<"auth"_n, auth> auth_table;

    void check_internal_auth (name self_contract, name action, string failure_identifier) {
        auth_table _auth(name(AUTHORITY_CONTRACT), self_contract.value);
        auto itr = _auth.find(action.value);
        check(itr != _auth.end(), failure_identifier + "no entry in authority table for this action and contract");
        auto authorized_contracts = itr->authorized_contracts;
        for(auto i = 0 ; i < authorized_contracts.size(); i++ ) {
            if(has_auth(authorized_contracts[i])) {
                return;
            }
        }
        check(false, failure_identifier + "Calling contract not in authorized list of accounts for action " + action.to_string());
    }  
};

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <vector>
#include <eosio/system.hpp>

#include "orginterface.hpp" 
#include "authorityinterface.hpp" 

using namespace eosio;
using namespace std;
using namespace org_contract;
using namespace authority_contract;

#define SIMPLEBADGE_CONTRACT "simplebaddev"

CONTRACT requests : public contract {
public:
    using contract::contract;

   ACTION ingestsimple(
    name originating_contract,
    name originating_contract_key,
    name requester, 
    vector<name> approvers, 
    name to, 
    symbol badge_symbol, 
    uint64_t amount,
    string memo,
    string request_memo,
    time_point_sec expiration_time);

   ACTION initseq(name org, name key, uint64_t seq_id);

   ACTION approve(name approver, name org, uint64_t request_id, string stream_reason);
   ACTION reject(name approver, name org, uint64_t request_id, string stream_reason);

   ACTION evidence(name authorized, name org, uint64_t request_id, string stream_reason);
   ACTION withdraw(name authorized, name org, uint64_t request_id, string stream_reason);

   ACTION sharestatus(name requester, uint64_t request_id, name originating_contract, name originating_contract_key, name old_status, name new_status);
   ACTION processone(name org, uint64_t request_id);
   ACTION process(name org, uint16_t batch_size);
private:

    TABLE request {
        uint64_t request_id;
        name action;
        name requester;
        name originating_contract;
        name originating_contract_key;
        vector<pair<name, name>> approvers;
        name status;
        time_point_sec expiration_time;

        auto primary_key() const { return request_id; }
        uint64_t by_status() const { return status.value; }
    };
    typedef multi_index<"requests"_n, request,
        indexed_by<"bystatus"_n, const_mem_fun<request, uint64_t, &request::by_status>>
    > request_index;

    TABLE simissue {
        uint64_t request_id;
        name to;
        symbol badge_symbol;
        uint64_t amount;
        string memo;

        auto primary_key() const { return request_id; }
    };
    typedef multi_index<"simissues"_n, simissue> simissue_index;

    TABLE sequence {
        name key;          // Primary key for the sequence
        uint64_t seq_id;   // The actual sequence ID

        auto primary_key() const { return key.value; }
    };
    typedef multi_index<"sequences"_n, sequence> sequence_index;


    struct sharestatus_args {
      name requester;
      uint64_t request_id;
      name originating_contract;
      name originating_contract_key;
      name old_status;
      name new_status;
    };

    struct simpleissue_args {
        name org;
        asset badge_asset;
        name to;
        string memo;       
    };
    
};

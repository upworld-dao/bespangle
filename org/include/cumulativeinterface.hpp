#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>

using namespace eosio;
using namespace std;

#define CUMULATIVE_CONTRACT "cumulativdev"

namespace cumulative_contract {
    TABLE account {
        asset    balance;
        uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };
    typedef eosio::multi_index<"accounts"_n, account> accounts;
}

#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>
#include "orginterface.hpp"
#include "authorityinterface.hpp"

using namespace eosio;
using namespace std;
using namespace org_contract;
using namespace authority_contract;

#define AUTHORITY_CONTRACT "authoritydev"
#define ORG_CONTRACT "organizatdev"
#define TOKEN_STAKER_CONTRACT ""
#define GOV_WEIGHT_CONTRACT ""

CONTRACT tokenstaker : public contract {
public:
    using contract::contract;

    /**
     * @brief Initialize staking parameters for an organization
     * 
     * @param org Organization name
     * @param token_symbol Symbol of the token to stake
     * @param lock_period_sec Seconds tokens are locked for after unstaking
     * @param governance_weight_ratio The ratio to convert staked tokens to governance weight
     */
    ACTION init(
        name org,
        symbol token_symbol,
        uint32_t lock_period_sec,
        double governance_weight_ratio
    );

    /**
     * @brief Update staking parameters for an organization
     */
    ACTION update(
        name org,
        symbol token_symbol,
        uint32_t lock_period_sec,
        double governance_weight_ratio
    );

    /**
     * @brief Stake tokens to the contract
     * 
     * @param org Organization name
     * @param account Account staking tokens
     * @param quantity Amount to stake
     * @param memo Memo for the transaction
     */
    ACTION stake(
        name org,
        name account,
        asset quantity,
        string memo
    );

    /**
     * @brief Unstake tokens from the contract (initiates the unstaking process)
     * 
     * @param org Organization name
     * @param account Account unstaking tokens
     * @param quantity Amount to unstake
     * @param memo Memo for the transaction
     */
    ACTION unstake(
        name org,
        name account,
        asset quantity,
        string memo
    );

    /**
     * @brief Claim unstaked tokens after lock period
     * 
     * @param org Organization name
     * @param account Account claiming tokens
     */
    ACTION claim(
        name org,
        name account
    );

    /**
     * @brief Handle token transfers to the contract (for staking)
     */
    [[eosio::on_notify("*::transfer")]] void on_transfer(
        name from,
        name to,
        asset quantity,
        string memo
    );

    /**
     * @brief Get the staked balance for an account
     * 
     * @param org Organization name
     * @param account Account to get balance for
     * @param token_symbol Symbol of the token
     * @return asset The staked balance
     */
    asset get_staked_balance(name org, name account, symbol token_symbol);

    /**
     * @brief Get the governance weight equivalent for staked tokens
     * 
     * @param org Organization name
     * @param account Account to get weight for
     * @param token_symbol Symbol of the token
     * @return double The governance weight
     */
    double get_governance_weight(name org, name account, symbol token_symbol);

private:
    TABLE stakeparams {
        name org;
        symbol token_symbol;
        uint32_t lock_period_sec;
        double governance_weight_ratio;  // How much each token contributes to governance weight
        time_point_sec last_updated;

        uint64_t primary_key() const { return org.value; }
        uint128_t by_org_symbol() const { return combine_keys(org.value, token_symbol.code().raw()); }

        static uint128_t combine_keys(uint64_t a, uint64_t b) {
            return (static_cast<uint128_t>(a) << 64) | b;
        }
    };
    typedef multi_index<"stakeparams"_n, stakeparams,
        indexed_by<"byorgsymbol"_n, const_mem_fun<stakeparams, uint128_t, &stakeparams::by_org_symbol>>
    > stakeparams_table;

    TABLE stakeentry {
        uint64_t id;
        name account;
        asset staked_amount;
        time_point_sec staked_at;

        uint64_t primary_key() const { return id; }
        uint64_t by_account() const { return account.value; }
        uint128_t by_acct_amount() const { return combine_keys(account.value, staked_amount.amount); }

        static uint128_t combine_keys(uint64_t a, uint64_t b) {
            return (static_cast<uint128_t>(a) << 64) | b;
        }
    };
    typedef multi_index<"stakeentries"_n, stakeentry,
        indexed_by<"byaccount"_n, const_mem_fun<stakeentry, uint64_t, &stakeentry::by_account>>,
        indexed_by<"byacctamount"_n, const_mem_fun<stakeentry, uint128_t, &stakeentry::by_acct_amount>>
    > stakeentries_table;

    TABLE unstakeentry {
        uint64_t id;
        name account;
        asset unstaked_amount;
        time_point_sec unstaked_at;
        time_point_sec claimable_at;

        uint64_t primary_key() const { return id; }
        uint64_t by_account() const { return account.value; }
        uint64_t by_claimable_at() const { return claimable_at.sec_since_epoch(); }
    };
    typedef multi_index<"unstakeentr"_n, unstakeentry,
        indexed_by<"byaccount"_n, const_mem_fun<unstakeentry, uint64_t, &unstakeentry::by_account>>,
        indexed_by<"byclaimable"_n, const_mem_fun<unstakeentry, uint64_t, &unstakeentry::by_claimable_at>>
    > unstakeentries_table;

    // Track the next stake entry ID
    TABLE nextids {
        uint64_t stake_id;
        uint64_t unstake_id;
    };
    typedef singleton<"nextids"_n, nextids> nextids_singleton;

    struct billing_args {
        name org;
        uint8_t actions_used;
    };

    void check_auth(name org);
    void add_stake(name org, name account, asset quantity);
    void process_unstake(name org, name account, asset quantity);
    void send_tokens(name to, asset quantity, string memo);
    uint64_t get_next_stake_id();
    uint64_t get_next_unstake_id();
}; 

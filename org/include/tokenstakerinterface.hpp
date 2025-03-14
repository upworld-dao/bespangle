#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

using namespace eosio;
using namespace std;

namespace tokenstaker_contract {

    class interface {
    private:
        name contract_account;
        name self_account;

    public:
        interface(name contract_account, name self_account)
            : contract_account(contract_account), self_account(self_account) {}

        /**
         * @brief Initialize the staking parameters for an organization
         */
        void init(
            name org,
            symbol token_symbol,
            uint32_t lock_period_sec,
            double governance_weight_ratio
        ) {
            action(
                permission_level{self_account, "active"_n},
                contract_account,
                "init"_n,
                std::make_tuple(org, token_symbol, lock_period_sec, governance_weight_ratio)
            ).send();
        }

        /**
         * @brief Stake tokens for an organization
         */
        void stake(
            name org,
            name account,
            asset quantity,
            string memo
        ) {
            action(
                permission_level{self_account, "active"_n},
                contract_account,
                "stake"_n,
                std::make_tuple(org, account, quantity, memo)
            ).send();
        }

        /**
         * @brief Unstake tokens from an organization
         */
        void unstake(
            name org,
            name account,
            asset quantity,
            string memo
        ) {
            action(
                permission_level{self_account, "active"_n},
                contract_account,
                "unstake"_n,
                std::make_tuple(org, account, quantity, memo)
            ).send();
        }

        /**
         * @brief Claim unstaked tokens after lock period
         */
        void claim(
            name org,
            name account
        ) {
            action(
                permission_level{self_account, "active"_n},
                contract_account,
                "claim"_n,
                std::make_tuple(org, account)
            ).send();
        }

        /**
         * @brief Get the staked balance for an account
         * 
         * @param org Organization name
         * @param account Account to get staked balance for
         * @return asset The staked balance
         */
        asset get_staked_balance(name org, name account, symbol token_symbol) {
            // This is a view function and cannot be implemented in the interface
            // The calling contract should use a table lookup to get this value
            return asset(0, token_symbol); // Placeholder
        }
    };

    // Table definitions for external contracts to access
    struct stakeparams {
        name org;
        symbol token_symbol;
        uint32_t lock_period_sec;
        double governance_weight_ratio;
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

    struct stakeentry {
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

    struct unstakeentry {
        uint64_t id;
        name account;
        asset unstaked_amount;
        time_point_sec unstaked_at;
        time_point_sec claimable_at;

        uint64_t primary_key() const { return id; }
        uint64_t by_account() const { return account.value; }
        uint64_t by_claimable_at() const { return claimable_at.sec_since_epoch(); }

        static uint128_t combine_keys(uint64_t a, uint64_t b) {
            return (static_cast<uint128_t>(a) << 64) | b;
        }
    };
    typedef multi_index<"unstakeentr"_n, unstakeentry,
        indexed_by<"byaccount"_n, const_mem_fun<unstakeentry, uint64_t, &unstakeentry::by_account>>,
        indexed_by<"byclaimable"_n, const_mem_fun<unstakeentry, uint64_t, &unstakeentry::by_claimable_at>>
    > unstakeentries_table;
} 

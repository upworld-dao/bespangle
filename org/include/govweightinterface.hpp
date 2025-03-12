#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

namespace govweight_contract {

    class interface {
    private:
        name contract_account;
        name self_account;

    public:
        interface(name contract_account, name self_account)
            : contract_account(contract_account), self_account(self_account) {}

        /**
         * @brief Initialize the governance weight parameters
         */
        void init(
            name org,
            symbol gov_badge_symbol,
            uint8_t cumulative_weight,
            uint8_t bounded_weight,
            uint8_t staked_token_weight,
            uint16_t seasons_count,
            uint64_t min_balance,
            symbol token_symbol
        ) {
            action(
                permission_level{self_account, "active"_n},
                contract_account,
                "init"_n,
                std::make_tuple(org, gov_badge_symbol, cumulative_weight, bounded_weight, staked_token_weight, seasons_count, min_balance, token_symbol)
            ).send();
        }

        /**
         * @brief Update the governance weight parameters
         */
        void update(
            name org,
            symbol gov_badge_symbol,
            uint8_t cumulative_weight,
            uint8_t bounded_weight,
            uint8_t staked_token_weight,
            uint16_t seasons_count,
            uint64_t min_balance,
            symbol token_symbol
        ) {
            action(
                permission_level{self_account, "active"_n},
                contract_account,
                "update"_n,
                std::make_tuple(org, gov_badge_symbol, cumulative_weight, bounded_weight, staked_token_weight, seasons_count, min_balance, token_symbol)
            ).send();
        }

        /**
         * @brief Calculate governance weight for a specific account
         */
        void calculate_weight(
            name org,
            name account
        ) {
            action(
                permission_level{self_account, "active"_n},
                contract_account,
                "calcweight"_n,
                std::make_tuple(org, account)
            ).send();
        }

        /**
         * @brief Get the governance weight for an account (view-only function)
         * 
         * @param org Organization name
         * @param account Account to get weight for
         * @return double The governance weight
         */
        double get_weight(name org, name account) {
            // This is a view function and cannot be implemented in the interface
            // The calling contract should use a table lookup to get this value
            return 0.0; // Placeholder
        }
    };

    // Table definitions for external contracts to access
    struct govparams {
        name org;
        symbol gov_badge_symbol;
        uint8_t cumulative_weight;     // 0-100 percentage
        uint8_t bounded_weight;        // 0-100 percentage
        uint8_t staked_token_weight;   // 0-100 percentage
        uint16_t seasons_count;        // number of seasons to consider
        uint64_t min_balance;          // minimum balance required
        symbol token_symbol;           // symbol of the token used for staking
        time_point_sec last_updated;   // timestamp of last parameter update

        uint64_t primary_key() const { return org.value; }
        uint128_t by_org_badge() const { return combine_keys(org.value, gov_badge_symbol.code().raw()); }

        static uint128_t combine_keys(uint64_t a, uint64_t b) {
            return (static_cast<uint128_t>(a) << 64) | b;
        }
    };
    typedef multi_index<"govparams"_n, govparams,
        indexed_by<"byorgbadge"_n, const_mem_fun<govparams, uint128_t, &govparams::by_org_badge>>
    > govparams_table;

    struct weightcache {
        name account;
        double weight;
        time_point_sec calculated_at;

        uint64_t primary_key() const { return account.value; }
    };
    typedef multi_index<"weightcache"_n, weightcache> weightcache_table;
} 

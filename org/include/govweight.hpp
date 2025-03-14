#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <cmath>
#include "orginterface.hpp"
#include "badgedatainterface.hpp"
#include "boundedagginterface.hpp"
#include "cumulativeinterface.hpp"
#include "tokenstakerinterface.hpp"

using namespace eosio;
using namespace std;
using namespace org_contract;
using namespace badgedata_contract;
using namespace boundedagg_contract;
using namespace cumulative_contract;
using namespace tokenstaker_contract;

#define AUTHORITY_CONTRACT "authoritydev"
#define ORG_CONTRACT "organizatdev"
#define BADGEDATA_CONTRACT "badgedatadev"
#define BOUNDED_AGG_CONTRACT "boundedagdev"
#define CUMULATIVE_CONTRACT "cumulativdev"
#define GOV_WEIGHT_CONTRACT ""
#define TOKEN_STAKER_CONTRACT ""

CONTRACT govweight : public contract {
public:
    using contract::contract;

    /**
     * @brief Initialize the governance weight calculation parameters
     * 
     * @param org Organization name
     * @param gov_badge_symbol Symbol of the governance badge
     * @param cumulative_weight Weight to apply to cumulative balance (0-100%)
     * @param bounded_weight Weight to apply to bounded aggregate balance (0-100%)
     * @param staked_token_weight Weight to apply to staked tokens (0-100%)
     * @param seasons_count Number of seasons to calculate over
     * @param min_balance Minimum balance required to have voting power
     * @param token_symbol Symbol of the token used for staking
     */
    ACTION init(
        name org,
        symbol gov_badge_symbol,
        uint8_t cumulative_weight,
        uint8_t bounded_weight,
        uint8_t staked_token_weight,
        uint16_t seasons_count,
        uint64_t min_balance,
        symbol token_symbol
    );

    /**
     * @brief Update the governance weight calculation parameters
     */
    ACTION update(
        name org,
        symbol gov_badge_symbol,
        uint8_t cumulative_weight,
        uint8_t bounded_weight,
        uint8_t staked_token_weight,
        uint16_t seasons_count,
        uint64_t min_balance,
        symbol token_symbol
    );

    /**
     * @brief Calculate governance weight for a specific account
     * 
     * @param org Organization name
     * @param account Account to calculate weight for
     * @return double The calculated governance weight
     */
    ACTION calcweight(
        name org,
        name account
    );

    /**
     * @brief Get the governance weight for an account (view-only function)
     * 
     * @param org Organization name
     * @param account Account to get weight for
     * @return double The governance weight
     */
    double get_weight(name org, name account);

private:
    TABLE govparams {
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

    TABLE weightcache {
        name account;
        double weight;
        time_point_sec calculated_at;

        uint64_t primary_key() const { return account.value; }
    };
    typedef multi_index<"weightcache"_n, weightcache> weightcache_table;

    struct billing_args {
        name org;
        uint8_t actions_used;
    };

    void check_auth(name org);
    double calculate_weight(name org, name account, const govparams& params);
    asset get_cumulative_balance(name org, name account, symbol badge_symbol);
    asset get_bounded_agg_balance(name org, name account, symbol badge_symbol, uint16_t seasons_count);
    double get_staked_token_weight(name org, name account, symbol token_symbol);
}; 

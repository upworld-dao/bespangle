#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include "orginterface.hpp"
#include "andemitterinterface.hpp"
#include "authorityinterface.hpp"

using namespace std;
using namespace eosio;
using namespace org_contract;
using namespace andemitter_contract;
using namespace authority_contract;

#define REQUESTS_CONTRACT "requestsdevd"
#define SIMPLEBADGE_CONTRACT "simplebaddev"
#define BADGEDATA_CONTRACT "badgedatadev"
#define CUMULATIVE_CONTRACT "cumulativdev"
#define STATISTICS_CONTRACT "statisticdev"

#define STATUS_CHANGE_NOTIFICATION REQUESTS_CONTRACT"::sharestatus"

CONTRACT bounties : public contract {
  public:
    using contract::contract;

    struct contract_asset {
        name contract;
        asset emit_asset;
    };

    ACTION addactionauth (name org, name action, name authorized_account);

    ACTION delactionauth (name org, name action, name authorized_account);



   /**
    * @brief Create a new bounty in the initial state.
    *
    * This action creates a new bounty with the three new columns to classify the bounty type.
    * The badge will be set by either oldbadge or newbadge actions subsequently.
    *
    * @param authorized                              The account authorized to create the bounty.
    * @param emission_symbol                         The symbol identifying the bounty record.
    * @param payer                                   The account paying for the bounty.
    * @param bounty_display_name                     The display name for the bounty.
    * @param bounty_ipfs_description                 The IPFS hash for the bounty description.
    * @param emit_assets                             The assets to emit.
    * @param total_fungible_assets                   The total fungible assets for the bounty.
    * @param max_fungible_assets_payout_per_winner   The maximum fungible assets payout per winner.
    * @param max_submissions_per_participant         The maximum number of submissions per participant.
    * @param reviewers                               A vector of reviewer names.
    * @param participation_start_time                The start time for participation.
    * @param participation_end_time                  The end time for participation.
    * @param bounty_settlement_time                  The settlement time for the bounty.
    * @param new_or_old_badge                        Whether to use a new or old badge ("new"_n or "old"_n).
    * @param limited_or_unlimited_participants       Whether participants are limited or unlimited ("limited"_n or "unlimited"_n).
    * @param open_or_closed_external_participation_list Whether the participation list is open, closed, or external ("open"_n, "closed"_n, or "external"_n).
    */
   ACTION newbounty(name authorized, 
                  symbol emission_symbol,
                        name payer,
                        string bounty_display_name, 
                        string bounty_ipfs_description,
                        vector<asset> emit_assets,
                        vector<extended_asset> total_fungible_assets,
                        vector<extended_asset> max_fungible_assets_payout_per_winner,
                        uint16_t max_submissions_per_participant,
                        vector<name> reviewers,
                        time_point_sec participation_start_time,
                        time_point_sec participation_end_time,
                        time_point_sec bounty_settlement_time,
                        name new_or_old_badge,
                        name limited_or_unlimited_participants,
                        name open_or_closed_external_participation_list);

   /**
    * @brief Set an existing badge for a bounty.
    *
    * This action updates the bounty with an existing badge symbol.
    * Can only be executed when new_or_old_badge is set to "old".
    *
    * @param authorized        The account authorized to modify the bounty.
    * @param emission_symbol   The symbol identifying the bounty record.
    * @param old_badge_symbol  The symbol of the existing badge to use.
    */
   ACTION oldbadge(name authorized, symbol emission_symbol, symbol old_badge_symbol);

   /**
    * @brief Create and set a new badge for a bounty.
    *
    * This action creates a new badge and updates the bounty with its symbol.
    * Can only be executed when new_or_old_badge is set to "new".
    *
    * @param authorized          The account authorized to modify the bounty.
    * @param emission_symbol     The symbol identifying the bounty record.
    * @param new_badge_symbol    The symbol for the new badge.
    * @param badge_display_name  The display name for the new badge.
    * @param badge_ipfs_image    The IPFS hash for the badge image.
    * @param badge_description   The description for the new badge.
    * @param badge_creation_memo The memo for the badge creation.
    * @param lifetime_aggregate  Whether the badge is lifetime aggregate.
    * @param lifetime_stats      Whether the badge is lifetime stats.
    */
   ACTION newbadge(name authorized, symbol emission_symbol, symbol new_badge_symbol,
                          string badge_display_name, string badge_ipfs_image,
                          string badge_description, string badge_creation_memo, bool lifetime_aggregate, bool lifetime_stats);

   /**
    * @brief Set the maximum number of participants for a limited bounty.
    *
    * This action updates the bounty with the maximum number of participants.
    * Can only be executed when limited_or_unlimited_participants is set to "limited".
    *
    * @param authorized                The account authorized to modify the bounty.
    * @param emission_symbol           The symbol identifying the bounty record.
    * @param max_number_of_participants The maximum number of participants allowed.
    */
   ACTION limited(name authorized, symbol emission_symbol, uint64_t max_number_of_participants);

   /**
    * @brief Set the closed participants list for a bounty.
    *
    * This action updates the bounty with a closed list of participants.
    * Can only be executed when open_or_closed_external_participation_list is set to "closed".
    *
    * @param authorized        The account authorized to modify the bounty.
    * @param emission_symbol   The symbol identifying the bounty record.
    * @param participants      A vector of participant names allowed to participate.
    */
   ACTION closed(name authorized, symbol emission_symbol, vector<name> participants);

   /**
    * @brief Set external participant check information for a bounty.
    *
    * This action updates the bounty with external contract information for participant validation.
    * Can only be executed when open_or_closed_external_participation_list is set to "external".
    *
    * @param authorized                  The account authorized to modify the bounty.
    * @param emission_symbol             The symbol identifying the bounty record.
    * @param participant_check_contract  The contract that will check participant validity.
    * @param participant_check_action    The action on the contract that will check participant validity.
    * @param participant_check_scope     The scope for the participant check.
    */
   ACTION external(name authorized, symbol emission_symbol, 
                  name participant_check_contract, 
                  name participant_check_action, 
                  name participant_check_scope);

   /**
    * @brief Add new reviewers to an existing bounty.
    *
    * This action looks up the bounty record by `emission_symbol` and adds each
    * reviewer from the input vector to the bounty's reviewers vector if they are not already present.
    *
    * @param authorized      The account authorized to modify the bounty.
    * @param emission_symbol The symbol identifying the bounty record.
    * @param reviewers       A vector of reviewer names to add.
    */
   ACTION reviewers(name authorized, symbol emission_symbol, vector<name> reviewers);


   /**
    * @brief Add new participants to an existing bounty.
    *
    * This action looks up the bounty record by `emission_symbol` and then adds each
    * participant from the input vector to the bounty's participants map if they are not already present.
    * **This action only proceeds if `closed_participants` is true.**
    *
    * @param authorized      The account authorized to modify the bounty.
    * @param emission_symbol The symbol identifying the bounty record.
    * @param participants    A vector of participant names to add.
    */
   ACTION participants(name authorized, symbol emission_symbol, vector<name> participants);
    

   
   [[eosio::on_notify("*::transfer")]]
   void ontransfer(name from, name to, asset amount, string memo);


   ACTION signup(name participant, symbol emission_symbol, string stream_reason);
   ACTION cancelsignup(name participant, symbol emission_symbol, string stream_reason);   
   ACTION submit(name participant, symbol emission_symbol, string stream_reason);

   ACTION closebounty(symbol emission_symbol);
   ACTION withdraw(name account, symbol emission_symbol);// returns refund

   ACTION cleanup(symbol emission_symbol);

   [[eosio::on_notify(STATUS_CHANGE_NOTIFICATION)]]
   void status(name requester, uint64_t request_id, name originating_contract, name originating_contract_key, name old_status, name new_status);


    
  private:

   // Struct definitions for inline action arguments
   void handle_withdrawal(symbol target_sym, name requester); 
   // For calling nextemission in the andemitter contract.
   struct nextemission_args {
      name org;
   };

   // For calling initsimple in the simmanager contract.
    struct createsimple_args {
      name org;
      symbol badge_symbol;
      string display_name;
      string ipfs_image;
      string description;
      string memo;
    };

   // For calling newemission in the aemanager contract.
   struct new_emission_args {
      name org;
      symbol emission_symbol;
      string display_name; 
      string ipfs_description;
      vector<asset> emitter_criteria;
      vector<contract_asset> emit_assets;
      bool cyclic;
   };

   struct activate_emission_args {
      name org;
      symbol emission_symbol;
   };

   struct deactivate_emission_args {
      name org;
      symbol emission_symbol;
   };

   struct addfeature_args {
      name org;
      symbol badge_symbol;
      name notify_account;
      string memo;
   };

   // Bounty table definition with new state_counts column.
   TABLE bounty {
      symbol                     emission_symbol;
      string                     bounty_display_name;
      string                     bounty_ipfs_description;
      vector<contract_asset>     emit_contract_assets;
      symbol                     badge_symbol;
      name                       status;
      vector<extended_asset>     total_fungible_assets;
      vector<extended_asset>     total_fungible_assets_deposited;
      uint16_t                   max_submissions_per_participant;
      uint64_t                   max_number_of_participants;
      uint64_t                   num_participants = 0;
      vector<extended_asset>     max_fungible_assets_payout_per_winner;

      name                       participant_check_contract;
      name                       participant_check_action;
      name                       participant_check_scope;

      // New columns for the refactored actions
      name                       new_or_old_badge;                          // Values: "new"_n or "old"_n
      name                       limited_or_unlimited_participants;         // Values: "limited"_n or "unlimited"_n
      name                       open_or_closed_or_external_participation_list; // Values: "open"_n, "closed"_n, or "external"_n

      // Participants map: when a participant signs up, we record an entry.
      // For closed participation, they must already be present.
      // The value is set to 1 on sign‐up.
      map<name, uint64_t>        participants;
      // Submissions map: records the number of submissions per participant.
      map<name, uint64_t>        submissions;
      vector<name>               reviewers;
      time_point_sec             participation_start_time;
      time_point_sec             participation_end_time;
      time_point_sec             bounty_settlement_time;
      map<name, uint64_t>        state_counts; // New column for state counts
      name                       payer;

      uint64_t primary_key() const { return emission_symbol.code().raw(); }
   };
   typedef multi_index<"bounty"_n, bounty> bounty_table;

   // TABLE: bountypool
   // ------------------------------
   // This table is scoped by the bounty's emission symbol code.
   TABLE bountypool {
      name account; // Depositing account (non-payer deposits)
      vector<extended_asset> total_fungible_assets_deposited;
      
      uint64_t primary_key() const { return account.value; }
   };
   // When instantiating bountypool_table, the scope is set to the bounty's symbol code.
   typedef multi_index<"bountypool"_n, bountypool> bountypool_table;

   // ------------------------------
   // TABLE: settings
   // ------------------------------
   // Stores various configuration settings.
   TABLE settings {
      name key;
      uint64_t value;
      
      uint64_t primary_key() const { return key.value; }
   };
   typedef multi_index<"settings"_n, settings> settings_table;

   TABLE actionauths {
      name action;
      vector<name> authorized_accounts;
      auto primary_key() const { return action.value; }
    };
    typedef multi_index<name("actionauths"), actionauths> actionauths_table;

   void distribute(symbol badge_symbol, name account) {
      
      // Lookup the bounty record.
      bounty_table bounty_tbl(get_self(), get_self().value);
      auto b_itr = bounty_tbl.find(badge_symbol.code().raw());
      check(b_itr != bounty_tbl.end(), "Bounty record not found.");

      // Retrieve the approved count from state_counts (using key "approved"_n).
      auto appr_itr = b_itr->state_counts.find("approved"_n);
      check(appr_itr != b_itr->state_counts.end(), "Approved count not found in state_counts.");
      uint64_t approved = appr_itr->second;
      check(approved > 0, "Approved count must be greater than zero.");

      // Retrieve fee percentage from settings table.
      settings_table settings(get_self(), get_self().value);
      auto fee_itr = settings.find("fees"_n.value);
      uint64_t fee_bp = 0; // fee in basis points (e.g., 100 for 1%)
      if (fee_itr != settings.end()) {
         fee_bp = fee_itr->value;
      }

      // Vectors to accumulate net amounts and fee amounts for transfer.
      vector<extended_asset> net_assets;
      vector<extended_asset> fee_assets;

      // Lambda to process deposit records.
      // The boolean flag 'applyCap' indicates whether to enforce the cap from max_fungible_assets_payout_per_winner.
      auto process_deposits = [&](const vector<extended_asset>& deposits, bool applyCap) {
         for (auto const &dep : deposits) {
            // Verify the deposit asset is one of the bounty's target assets.
            bool valid = false;
            for (auto const &target : b_itr->total_fungible_assets) {
               if (target.quantity.symbol == dep.quantity.symbol && target.contract == dep.contract) {
                  valid = true;
                  break;
               }
            }
            check(valid, "Deposit asset not recognized in bounty target assets.");
            // Compute the distribution amount.
            int64_t total_deposited = dep.quantity.amount;
            int64_t computed = total_deposited / approved;  // integer division
            int64_t final_amt = computed;
            if (applyCap) {
               // Look up the cap for this asset from max_fungible_assets_payout_per_winner.
               int64_t max_allowed = 0;
               bool found_cap = false;
               for (auto const &cap_ext : b_itr->max_fungible_assets_payout_per_winner) {
                  if (cap_ext.quantity.symbol == dep.quantity.symbol && cap_ext.contract == dep.contract) {
                     max_allowed = cap_ext.quantity.amount;
                     found_cap = true;
                     break;
                  }
               }
               if (found_cap && final_amt > max_allowed) {
                  final_amt = max_allowed;
               }
            }
            // Apply fees.
            int64_t fee_amt = (final_amt * fee_bp) / 10000;  // fee fraction = fee_bp/10000
            int64_t net_amt = final_amt - fee_amt;
            if (net_amt > 0) {
               asset net_asset = asset(net_amt, dep.quantity.symbol);
               extended_asset ext_net { net_asset, dep.contract };
               net_assets.push_back(ext_net);
            }
            if (fee_amt > 0) {
               asset fee_asset = asset(fee_amt, dep.quantity.symbol);
               extended_asset ext_fee { fee_asset, dep.contract };
               fee_assets.push_back(ext_fee);
            }
         }
      };

      process_deposits(b_itr->total_fungible_assets_deposited, true);
      bountypool_table pool_tbl(get_self(), badge_symbol.code().raw());
      auto pool_itr = pool_tbl.begin();
      while(pool_itr != pool_tbl.end()) {
         process_deposits(pool_itr->total_fungible_assets_deposited, false);
         pool_itr ++;
      }

      // For each asset computed, perform two transfers:
      // one to the target account (net amount) and one to the treasury account (fee amount).
      for (auto const &net_ext : net_assets) {
         action(
            permission_level{ get_self(), "active"_n },
            net_ext.contract,
            "transfer"_n,
            std::make_tuple(get_self(), account, net_ext.quantity, string("Distribution from bounty"))
         ).send();
      }
      for (auto const &fee_ext : fee_assets) {
         action(
            permission_level{ get_self(), "active"_n },
            fee_ext.contract,
            "transfer"_n,
            std::make_tuple(get_self(), "treasury"_n, fee_ext.quantity, string("Fee from distribution"))
         ).send();
      }
   }


   // ------------------------------
   // Remote Action Struct for ingestsimple (from the "requests" contract)
   // ------------------------------
   struct ingestsimple_args {
      name           originating_contract;
      name           originating_contract_key;
      name           requester;
      vector<name>   approvers;
      name           to;
      symbol         badge_symbol;
      uint64_t       amount;
      string         memo;
      string         request_memo;
      time_point_sec expiration_time;
   };


   void deactivate_emission(symbol emission_symbol) {
      string action_name = "deactivate";
      string failure_identifier = "CONTRACT: aemanager, ACTION: " + action_name + ", MESSAGE: ";
      name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);

      action{
         permission_level{get_self(), name("active")},
         name(ANDEMITTER_CONTRACT),
         name("deactivate"),
         deactivate_emission_args{
            .org = org,
            .emission_symbol = emission_symbol
         }
      }.send();
   }

   // Check if an account is authorized for a specific action
   bool has_action_authority(name org, name action_name, name account) {
       // If the account is the organization itself, it's always authorized
       if (account == org) {
           return true;
       }
       
       // Check if there's a specific authorization for this action
       actionauths_table _actionauths(get_self(), org.value);
       auto itr = _actionauths.find(action_name.value);
       
       // If no specific authorization exists, only the org itself is authorized
       if (itr == _actionauths.end()) {
           return false;
       }
       
       // Check if the account is in the list of authorized accounts
       return std::find(itr->authorized_accounts.begin(), itr->authorized_accounts.end(), account) 
              != itr->authorized_accounts.end();
   }

   // Helper function to check if setup is complete and update status
   void check_and_update_status(bounty& row);

};

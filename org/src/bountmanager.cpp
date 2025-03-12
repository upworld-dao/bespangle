#include <bounties.hpp>
  
  
   ACTION bounties::newbounty( name             authorized,
                        name                       payer,

                        symbol                     badge_symbol,
                        bool                       new_badge,
                        string                     badge_display_name, 
                        string                     badge_ipfs_image,
                        string                     badge_description, 
                        string                     badge_creation_memo,
                        bool                       lifetime_aggregate,
                        bool                       lifetime_stats,

                        string                     bounty_display_name, 
                        string                     bounty_ipfs_description,
                        vector<asset>              emit_assets,

                        vector<extended_asset>     total_fungible_assets,

                        bool                       capped_participation,
                        uint16_t                   max_submissions_per_participant,
                        uint64_t                   max_number_of_participants,
                        vector<extended_asset>     max_fungible_assets_payout_per_winner,

                        bool                       closed_participants,
                        bool                       external_participant_check;
                        name                       participant_check_contract;
                        name                       participant_check_action;
                        name                       participant_check_scope;
                        vector<name>               participants_list,
                        vector<name>               reviewers,
                        time_point_sec             participation_start_time,
                        time_point_sec             participation_end_time,
                        time_point_sec             bounty_settlement_time ) {
      // 1. Authorization: Ensure the authorized is authorized.
      require_auth( authorized );
      string action_name = "newbounty";
      string failure_identifier = "CONTRACT: bounties, ACTION: " + action_name + ", MESSAGE: ";

      // 2. Time validations.
      time_point_sec now = current_time_point();
      check( participation_start_time > now, "participation_start_time must be in the future" );
      check( participation_end_time > now, "participation_end_time must be in the future" );
      check( bounty_settlement_time > now, "bounty_settlement_time must be in the future" );
      check( participation_end_time > participation_start_time, "participation_end_time must be greater than participation_start_time" );
      check( bounty_settlement_time > participation_end_time, "bounty_settlement_time must be greater than participation_end_time" );

      // 3. Validate offchain/onchain lookup data based on new_badge.
      if( !new_badge ) {
         check( badge_display_name.empty(), "badge_display_name must be empty if new_badge is false" );
         check( badge_ipfs_image.empty(), "badge_ipfs_image must be empty if new_badge is false" );
         check( badge_description.empty(), "badge_description must be empty if new_badge is false" );
         check( badge_creation_memo.empty(), "badge_creation_memo must be empty if new_badge is false" );
      }

      // 2. Validate capped participation fields.
      if( capped_participation ) {
         check( max_submissions_per_participant > 0, "max_submissions_per_participant must be > 0 when capped_participation is true" );
         check( max_number_of_participants > 0, "max_number_of_participants must be > 0 when capped_participation is true" );
         check( max_fungible_assets_payout_per_winner.size() > 0, "max_fungible_assets_payout_per_winner must have at least one asset when capped_participation is true" );
         check( total_fungible_assets.size() == max_fungible_assets_payout_per_winner.size(), "There must be a 1:1 match between total_fungible_assets and max_fungible_assets_payout_per_winner" );

      } else {
         check( max_submissions_per_participant == 0, "max_submissions_per_participant must be 0 when capped_participation is false" );
         check( max_number_of_participants == 0, "max_number_of_participants must be 0 when capped_participation is false" );
         check( max_fungible_assets_payout_per_winner.empty(), "max_fungible_assets_payout_per_winner must be empty when capped_participation is false" );
      }

      // --- Validate payout amounts if participation is capped ---
      if( capped_participation ) {
         uint64_t factor = max_submissions_per_participant * max_number_of_participants;
         for ( auto& payout : max_fungible_assets_payout_per_winner ) {
            bool found = false;
            for ( auto& total : total_fungible_assets ) {
               if( total.quantity.symbol == payout.quantity.symbol && total.contract == payout.contract ) {
                  found = true;
                  int64_t allowed = total.quantity.amount / factor;
                  check( payout.quantity.amount <= allowed, 
                         "Payout amount for asset " + payout.quantity.symbol.code().to_string() + " exceeds allowed maximum per winner (" + std::to_string(allowed) + ")" );
                  break;
               }
            }
            check( found, "Asset " + payout.quantity.symbol.code().to_string() + " in max_fungible_assets_payout_per_winner not found in total_fungible_assets" );
         }
      }
      // Extract org from badge_symbol
      name org = get_org_from_internal_symbol(badge_symbol, failure_identifier);

      // 3. Validate closed participants field.
      if( closed_participants ) {
         if (external_participant_check) {
            check(participant_check_contract.value != 0, "i dont know which contract has external participants list");
            check(participant_check_action.value != 0, "i dont which action to call for external participation check. action should be (name participant, name scope)");
            check(participant_check_scope.value != 0, "i dont what scope to pass for partipation check action.");
         }
      } else {
         check( participants_list.empty(), "participants_list must be empty when closed_participants is false" );
      }


      action(
         permission_level{ get_self(), "active"_n },
         name(ORG_CONTRACT),
         "nextemission"_n,
         make_tuple(org)
      ).send();

      string emission_code = get_onchain_lookup_system_data(org, "nextemission");
      symbol emission_symbol = symbol(org_code + emission_code, 0)


      map<name, uint64_t> unique_participants;
      for ( auto& p : participants_list ) {
         unique_participants[p] = 0;  // each unique participant appears with a count of 0
      }

      // 6. Insert the new bounty into the bounty table.
      bounty_table bounty_tbl( get_self(), get_self().value );
      bounty_tbl.emplace( get_self(), [&]( auto& row ) {
         row.emission_symbol                        = emission_symbol;
         row.badge_symbol                           = badge_symbol;
         row.status                                 = "init"_n; // initial status
         row.total_fungible_assets                  = total_fungible_assets;
         row.capped_participation                   = capped_participation;
         row.max_submissions_per_participant           = max_submissions_per_participant;
         row.max_number_of_participants             = max_number_of_participants;
         row.max_fungible_assets_payout_per_winner  = max_fungible_assets_payout_per_winner;
         row.closed_participants                    = closed_participants;
         row.external_participant_check             = external_participant_check;
         row.participant_check_contract             = participant_check_contract;
         row.participant_check_action               = participant_check_action;
         row.participant_check_scope                = participant_check_scope;
         row.participants                           = unique_participants;
         row.reviewers                              = reviewers;
         row.participation_start_time               = participation_start_time;
         row.participation_end_time                 = participation_end_time;
         row.bounty_settlement_time                 = bounty_settlement_time;
         row.state_counts                           = {};  // state_counts left blank (empty)
         row.payer                                  = payer;  
      });

   //   vector<extended_asset>     total_fungible_assets_deposited;

   //   uint64_t                   num_participants = 0;



      // 9. Call the "initsimple" action in the 'simmanager' contract only if new_badge is true.
      if( new_badge ) {

         action {
            permission_level{get_self(), name("active")},
            name(SIMPLEBADGE_CONTRACT),
            name("create"),
            createsimple_args {
               .org = org,
               .badge_symbol = badge_symbol,
               .display_name = badge_display_name,
               .ipfs_image = badge_ipfs_image,
               .description = badge_description,
               .memo = badge_creation_memo}
         }.send();

         vector<name> consumers;
         if(lifetime_aggregate) {
            consumers.push_back(name(CUMULATIVE_CONTRACT));
         }
         if(lifetime_aggregate && lifetime_stats) {
            consumers.push_back(name(STATISTICS_CONTRACT));
         } else if (!lifetime_aggregate && lifetime_stats) {
            check(false, "Enable Lifetime aggregates to capture Lifetime stats");
         }
    
         for (auto i = 0 ; i < consumers.size(); i++) {
            action {
            permission_level{get_self(), name("active")},
            name(ORCHESTRATOR_CONTRACT),
            name("addfeature"),
            addfeature_args {
               .org = org,
               .badge_symbol = badge_symbol,
               .notify_account = consumers[i],
               .memo = ""}
            }.send();
         }
      }

      // 8. Prepare for new emission:
      //    - The emitter criteria is set to one unit of the badge.
      //    - The cyclic flag is true if max entries per participant is greater than 1.
      asset criteria_asset = asset( 1, badge_symbol );
      vector<asset> emitter_criteria = { criteria_asset };
      bool cyclic = ( max_submissions_per_participant > 1 );

      vector<contract_asset> emit_contract_assets;

      for(asset a : emitter_criteria) {
         check(org == get_org_from_internal_symbol(a.symbol, failure_identifier), "org not same for all assets in emitter_criteria");
      }
      for(asset a : emit_assets) {
         check(org == get_org_from_internal_symbol(a.symbol, failure_identifier), "org not same for all assets in emit_badges");
      }

      for(auto i = 0 ; i < emit_assets.size(); i++) {
         emit_contract_assets.push_back (contract_asset {
            .contract = name(SIMPLEBADGE_CONTRACT),
            .emit_asset = emit_assets[i]
         });
      }

      action{
         permission_level{get_self(), name("active")},
         name(ANDEMITTER_CONTRACT),
         name("newemission"),
         new_emission_args{
            .org = org,
            .emission_symbol = emission_symbol,
            .display_name = bounty_display_name, 
            .ipfs_description = bounty_ipfs_description, 
            .emitter_criteria = emitter_criteria,
            .emit_assets = emit_contract_assets,
            .cyclic = cyclic
         }
      }.send();

      for (auto i = 0; i < emitter_criteria.size(); i++) {
         action{
            permission_level{get_self(), name("active")},
            name(BADGEDATA_CONTRACT),
            name("addfeature"),
            addfeature_args{
               .org = org,
               .badge_symbol = emitter_criteria[i].symbol,
               .notify_account = name(ANDEMITTER_CONTRACT),
               .memo = ""
            }
         }.send();
      }

      action{
         permission_level{get_self(), name("active")},
         name(ANDEMITTER_CONTRACT),
         name("activate"),
         activate_emission_args{
            .org = org,
            .emission_symbol = emission_symbol
         }
      }.send();

   }

   ACTION bounties::participants(name authorized, symbol emission_symbol, vector<name> participants) {
      // Ensure the authorized account signs the transaction.
      require_auth(authorized);

      // Open the bounty table.
      bounty_table bounty_tbl(get_self(), get_self().value);
      auto itr = bounty_tbl.find(emission_symbol.code().raw());
      check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol");

      // Only allow adding participants if closed_participants is true.
      check(itr->closed_participants, "cannot add participants when closed participants is false");
      check(!itr->external_participant_check, "cannot add participants when participants check is done in external contract");
      // Modify the record by adding new participants.
      bounty_tbl.modify(itr, get_self(), [&]( auto& row ) {
         for ( auto& p : participants ) {
            // If the participant is not already present, add it with a count of 1.
            if ( row.participants.find(p) == row.participants.end() ) {
               row.participants[p] = 1;
            }
         }
      });
   }

   ACTION bounties::reviewers(name authorized, symbol emission_symbol, vector<name> reviewers) {
      // Require that the authorized account signs the transaction.
      require_auth(authorized);

      // Open the bounty table.
      bounty_table bounty_tbl(get_self(), get_self().value);
      auto itr = bounty_tbl.find(emission_symbol.code().raw());
      check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol");

      // Modify the bounty record by appending reviewers that are not already in the vector.
      bounty_tbl.modify(itr, get_self(), [&](auto& row) {
         for (auto& rev : reviewers) {
            bool exists = false;
            // Check if the reviewer is already present.
            for (auto& existing_rev : row.reviewers) {
               if (existing_rev == rev) {
                  exists = true;
                  break;
               }
            }
            // Only add the reviewer if not already present.
            if (!exists) {
               row.reviewers.push_back(rev);
            }
         }
      });
   }
 
   void bounties::ontransfer(name from, name to, asset amount, string memo) {
      // Process only incoming transfers to this contract.
      if (to != get_self()) return;
      // Ignore transfers initiated by this contract.
      if (from == get_self()) return;

      // The memo is expected to contain the bounty identifier as the symbol code string.
      check(memo.size() > 0, "Memo must contain the bounty identifier (emission_symbol code string).");

      // Convert the memo string into a symbol, assuming precision 0.
      symbol target_sym = symbol(memo.c_str(), 0);
      uint64_t pk = target_sym.code().raw();

      bounty_table bounty_tbl(get_self(), get_self().value);
      auto bounty_itr = bounty_tbl.find(pk);
      check(bounty_itr != bounty_tbl.end(), "Bounty record not found for the given identifier.");

      // In both branches below, first verify that the transferred asset matches one in total_fungible_assets.
      bool asset_found = false;
      extended_asset target_asset;
      for (auto& ext : bounty_itr->total_fungible_assets) {
         if (ext.quantity.symbol == amount.symbol && ext.contract == get_first_receiver()) {
            asset_found = true;
            target_asset = ext;
            break;
         }
      }
      check(asset_found, "Transferred asset is not accepted by this bounty (not present in total_fungible_assets).");

      // Check if the depositor is the designated payer.
      if (from == bounty_itr->payer) {
         // ---------------------------
         // Payer Branch:
         // ---------------------------
         asset current_deposit = asset(0, amount.symbol);
         for (auto& ext : bounty_itr->total_fungible_assets_deposited) {
            if (ext.quantity.symbol == amount.symbol && ext.contract == get_first_receiver()) {
               current_deposit = ext.quantity;
               break;
            }
         }
         asset new_total = current_deposit + amount;
         if (new_total > target_asset.quantity) {
            // Refund the entire amount if it would exceed the target.
            action(
               permission_level{ get_self(), "active"_n },
               get_first_receiver(),
               "transfer"_n,
               std::make_tuple(get_self(), from, amount, string("Refund: deposit exceeds required amount"))
            ).send();
            return;
         }
         // Update the bounty record with the new deposit.
         bounty_tbl.modify(bounty_itr, get_self(), [&]( auto& row ) {
            bool found = false;
            for (auto& ext : row.total_fungible_assets_deposited) {
               if (ext.quantity.symbol == amount.symbol && ext.contract == get_first_receiver()) {
                  ext.quantity += amount;
                  found = true;
                  break;
               }
            }
            if (!found) {
               extended_asset new_ext;
               new_ext.quantity = amount;
               new_ext.contract = get_first_receiver();
               row.total_fungible_assets_deposited.push_back(new_ext);
            }
         });
         // Check whether all target assets have been fully deposited.
         bool all_deposited = true;
         for (auto& total_asset : bounty_itr->total_fungible_assets) {
            asset deposited = asset(0, total_asset.quantity.symbol);
            for (auto& ext : bounty_itr->total_fungible_assets_deposited) {
               if (ext.quantity.symbol == total_asset.quantity.symbol && ext.contract == total_asset.contract) {
                  deposited = ext.quantity;
                  break;
               }
            }
            if (deposited < total_asset.quantity) {
               all_deposited = false;
               break;
            }
         }
         if (all_deposited) {
            bounty_tbl.modify(bounty_itr, get_self(), [&]( auto& row ) {
               row.status = "deposited"_n;
            });
         }
      } else {
         // ---------------------------
         // Non-Payer Branch:
         // ---------------------------
         // Even for non-payer deposits, the asset and contract must match one of the target assets.
         // Additionally, a minimum deposit amount is required.
         // Retrieve the settings record for "minothers".
         settings_table settings(get_self(), get_self().value);
         auto s_itr = settings.find("minothers"_n.value);
         check(s_itr != settings.end(), "Settings record for minothers not found");
         uint64_t min_bp = s_itr->value; // This is in basis points, e.g., 1000 for 10%

         // Compute the minimum required deposit amount for this asset.
         int64_t min_required_amount = (target_asset.quantity.amount * min_bp) / 10000;
         check(amount.amount >= min_required_amount, "Deposit amount is less than the required minimum deposit");

         // Update the deposit in the bountypool table, scoped by the bounty's symbol code.
         bountypool_table pool_tbl(get_self(), target_sym.code().raw());
         auto pool_itr = pool_tbl.find(from.value);
         if (pool_itr == pool_tbl.end()) {
            // Create a new pool entry.
            pool_tbl.emplace(get_self(), [&]( auto& row ) {
               row.account = from;
               extended_asset ext;
               ext.quantity = amount;
               ext.contract = get_first_receiver();
               row.total_fungible_assets_deposited.push_back(ext);
            });
         } else {
            // Update the existing pool entry.
            pool_tbl.modify(pool_itr, same_payer, [&]( auto& row ) {
               bool found = false;
               for (auto& ext : row.total_fungible_assets_deposited) {
                  if (ext.quantity.symbol == amount.symbol && ext.contract == get_first_receiver()) {
                     ext.quantity += amount;
                     found = true;
                     break;
                  }
               }
               if (!found) {
                  extended_asset new_ext;
                  new_ext.quantity = amount;
                  new_ext.contract = get_first_receiver();
                  row.total_fungible_assets_deposited.push_back(new_ext);
               }
            });
         }
      }
   }

   void bounties::status(name requester, uint64_t request_id, name originating_contract, name originating_contract_key, name old_status, name new_status) {
      // Convert originating_contract_key (expected to be the bounty's emission symbol code as a string)
      // into a symbol with precision 0.
      symbol target_sym = symbol(originating_contract_key.to_string(), 0);
      uint64_t pk = target_sym.code().raw();

      bounty_table bounty_tbl(get_self(), get_self().value);
      auto itr = bounty_tbl.find(pk);
      check(itr != bounty_tbl.end(), "Bounty record not found for the given identifier.");

      bounty_tbl.modify(itr, get_self(), [&]( auto& row ) {
         // When processing a change from "approved" to "processed", do not subtract from "approved".
         // blank entry does not exist, so also not attempt to deduct when old_status is blank.
         if (old_status != name("blank") && !(old_status == "approved"_n && new_status == "processed"_n)) {
            auto old_itr = row.state_counts.find(old_status);
            if (old_itr != row.state_counts.end() && old_itr->second > 0) {
               row.state_counts[old_status] = old_itr->second - 1;
            }
         }
         // Increment the count for new_status.
         auto new_itr = row.state_counts.find(new_status);
         if (new_itr != row.state_counts.end()) {
            row.state_counts[new_status] = new_itr->second + 1;
         } else {
            row.state_counts[new_status] = 1;
         }
      });

      if(new_status == "processed"_n) {
         distribute(target_sym, requester);
      }
   }

   // ------------------------------
   // ACTION: signup
   // ------------------------------
   // Records a participant's sign-up.
   ACTION bounties::signup(name participant, symbol emission_symbol, string stream_reason) {

      require_auth(participant);

      bounty_table bounty_tbl(get_self(), get_self().value);
      auto itr = bounty_tbl.find(emission_symbol.code().raw());
      check(itr != bounty_tbl.end(), "Bounty record not found.");

      // Check that participation period has started and bounty status is "deposited".
      time_point_sec now = current_time_point();
      check(now >= itr->participation_start_time, "Participation period has not started yet.");
      check(now < itr->participation_end_time, "Participation period has ended.");
      check(itr->status == "deposited"_n, "Bounty is not in 'deposited' status.");

      if (itr->closed_participants && !itr->external_participant_check) {
         // For closed participation, the participant must already be present in the allowed list.
         auto p_itr = itr->participants.find(participant);
         check(p_itr != itr->participants.end(), "Participant not in closed participants list.");
         // Update the participant's value to 1 (indicating sign-up).
         if (itr->capped_participation) {
            check(itr->num_participants + 1 <= itr->max_number_of_participants, "Maximum number of participants exceeded.");
         }
         bounty_tbl.modify(itr, get_self(), [&](auto &row) {
            row.participants[participant] = 1;
            row.num_participants += 1;
         });
      } else if (itr->closed_participants && itr->external_participant_check) {
         
         action{
            permission_level{get_self(), name("active")},
            itr->participant_check_contract,
            itr->participant_check_action,
            std::make_tuple(itr->participant_check_scope, participant);
         }.send();
         if (itr->capped_participation) {
            check(itr->num_participants + 1 <= itr->max_number_of_participants, "Maximum number of participants exceeded.");
         }
         bounty_tbl.modify(itr, get_self(), [&](auto &row) {
            row.participants[participant] = 1;
            row.num_participants += 1;
         });
      } else {
         // For open participation: if participant is new, add them with value 1.
         if (itr->participants.find(participant) == itr->participants.end()) {
            if (itr->capped_participation) {
               check(itr->num_participants + 1 <= itr->max_number_of_participants, "Maximum number of participants exceeded.");
            }
            bounty_tbl.modify(itr, get_self(), [&](auto &row) {
               row.participants[participant] = 1;
               row.num_participants += 1;
            });
         }
         // If participant already exists, we assume they already signed up.
      }
      // (Optional: Record the stream_reason in an audit log.)
   }

   // ------------------------------
   // ACTION: cancelsignup
   // ------------------------------
   // Cancels a participant's sign-up if they have not yet submitted.
   ACTION bounties::cancelsignup(name participant, symbol emission_symbol, string stream_reason) {

      require_auth(participant);
      bounty_table bounty_tbl(get_self(), get_self().value);
      auto itr = bounty_tbl.find(emission_symbol.code().raw());
      check(itr != bounty_tbl.end(), "Bounty record not found.");

      // Verify that the participant has signed up.
      auto part_itr = itr->participants.find(participant);
      check(part_itr != itr->participants.end(), "Participant is not signed up.");

      // Ensure that no submission has been recorded.
      auto sub_itr = itr->submissions.find(participant);
      if (sub_itr != itr->submissions.end()) {
         check(sub_itr->second == 0, "Cannot cancel sign-up after submission has been done.");
      }

      bounty_tbl.modify(itr, get_self(), [&](auto &row) {
         if (row.closed_participants && !row.external_participant_check) {
            // For closed participation, update the participant's value to 0.
            row.participants[participant] = 0;
         } else {
            // For open participation, erase the participant entry.
            row.participants.erase(participant);

            // Decrement the overall participant count.
            check(row.num_participants > 0, "Number of participants is already zero.");
            row.num_participants -= 1;
         }
      });

      // (Optional) Record stream_reason in an audit log if desired.
   }

   // ------------------------------
   // ACTION: submit
   // ------------------------------
   // Invokes the remote "consumesimple" action on the "requests" contract.
   // Only allowed if the participant has signed up (their value in participants is 1).
   // Then updates the submissions map.
   ACTION bounties::submit(name participant, symbol emission_symbol, string stream_reason) {
      require_auth(participant);
      bounty_table bounty_tbl(get_self(), get_self().value);
      auto itr = bounty_tbl.find(emission_symbol.code().raw());
      check(itr != bounty_tbl.end(), "Bounty record not found.");

      // Re-check that participation period has started and bounty status is "deposited".
      time_point_sec now = current_time_point();
      check(now >= itr->participation_start_time, "Participation period has not started yet.");
      check(now < itr->participation_end_time, "Participation period has ended.");

      check(itr->status == "deposited"_n, "Bounty is not in 'deposited' status.");

      // Verify that the participant has signed up and that their flag is set to 1.
      auto part_itr = itr->participants.find(participant);
      check(part_itr != itr->participants.end() && part_itr->second == 1, "Participant has not signed up. Use participate action first.");

      // Update the submissions map.
      bounty_tbl.modify(itr, get_self(), [&](auto &row) {
         uint64_t current_subs = 0;
         auto sub_itr = row.submissions.find(participant);
         if (sub_itr != row.submissions.end()) {
            current_subs = sub_itr->second;
         }
         check(current_subs < row.max_submissions_per_participant, "Maximum submission entries exceeded for this participant.");
         row.submissions[participant] = current_subs + 1;
      });

      // Prepare remote action parameters.
      // Convert the bounty's emission_symbol.code() to a string, then to a name.
      name origin_key = name(itr->emission_symbol.code().to_string());

      consumesimple_args cs_args {
         .originating_contract = get_self(),
         .originating_contract_key = origin_key,
         .requester = participant,
         .approvers = itr->reviewers,
         .to = participant,
         .badge_symbol = itr->badge_symbol,
         .amount = 1,
         .memo = "completed bounty",
         .request_memo = stream_reason,
         .expiration_time = itr->bounty_settlement_time
      };

      action(
         permission_level{ get_self(), "active"_n },
         name(REQUESTS_CONTRACT),
         "consumesimple"_n,
         cs_args
      ).send();
   }



   // ------------------------------
   // ACTION: distribute
   // ------------------------------
   // This action distributes funds to a given account.
   // For deposits from the designated payer (from bounty.total_fungible_assets_deposited),
   // the computed distribution amount is capped by the corresponding entry in
   // max_fungible_assets_payout_per_winner.
   // For deposits from non-payer accounts (from the bountypool table), the computed amount is used directly.
   // Fees are applied as per the settings table ("fees" key); for each asset, the net amount and fee are computed,
   // and then two transfers are performed: one sending the net amount to the target account and one sending the fee amount
   // to the "treasury" account.


      // ------------------------------
   // ACTION: withdraw
   // ------------------------------
   // Withdraws funds for a given bounty and account.
   // - If the withdrawing account is the designated payer, funds are withdrawn from the
   //   bounty table's total_fungible_assets_deposited vector.
   //   Additionally, the bounty's status must be "closed".
   // - If the withdrawing account is not the payer, funds are withdrawn from the bountypool table,
   //   which is scoped by the badge symbol.
   ACTION bounties::withdraw(name account, symbol emission_symbol) {

      require_auth(account);
      // Lookup bounty record using emission_symbol as primary key.
      bounty_table bounty_tbl(get_self(), get_self().value);
      auto b_itr = bounty_tbl.find(emission_symbol.code().raw());
      check(b_itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol.");

      if (account == b_itr->payer) {
         // Withdrawing by the designated payer.
         // Enforce that the bounty status is closed.
         check(b_itr->status == "closed"_n, "Bounty status must be 'closed' for payer withdrawal.");

         // For each asset in total_fungible_assets_deposited, transfer the entire balance to the payer.
         for (auto const &ext : b_itr->total_fungible_assets_deposited) {
            action(
               permission_level{ get_self(), "active"_n },
               ext.contract,
               "transfer"_n,
               std::make_tuple(get_self(), account, ext.quantity, string("Payer withdrawal from bounty"))
            ).send();
         }
         // Clear the deposits from the bounty record.
         bounty_tbl.modify(b_itr, get_self(), [&](auto &row) {
            row.total_fungible_assets_deposited.clear();
         });
      } else {
         // Withdrawing by a non-payer account.
         // Use the bountypool table, which is scoped by badge_symbol.
         bountypool_table pool_tbl(get_self(), emission_symbol.code().raw());
         auto pool_itr = pool_tbl.find(account.value);
         check(pool_itr != pool_tbl.end(), "No deposit record found for this account in bountypool.");

         // For each asset deposited by this account, transfer it.
         for (auto const &ext : pool_itr->total_fungible_assets_deposited) {
            action(
               permission_level{ get_self(), "active"_n },
               ext.contract,
               "transfer"_n,
               std::make_tuple(get_self(), account, ext.quantity, string("Non-payer withdrawal from bounty pool"))
            ).send();
         }
         // Remove the record from the bountypool table.
         pool_tbl.erase(pool_itr);
      }
   }

      /**
    * @brief Mark a bounty as closed.
    *
    * This action looks up the bounty record identified by the provided badge_symbol.
    * If the value in state_counts for "approved" is equal to the value for "processed",
    * then the bounty status is updated to "closed".
    *
    * @param badge_symbol - the symbol of the bounty badge.
    */
   ACTION bounties::closebounty(symbol emission_symbol) {
      bounty_table bounty_tbl(get_self(), get_self().value);
      auto itr = bounty_tbl.find(emission_symbol.code().raw());
      check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol.");

      // Retrieve approved and processed counts (defaulting to 0 if not present).
      uint64_t approved = 0;
      uint64_t processed = 0;
      
      auto appr_itr = itr->state_counts.find("approved"_n);
      if (appr_itr != itr->state_counts.end()) {
         approved = appr_itr->second;
      }
      
      auto proc_itr = itr->state_counts.find("processed"_n);
      if (proc_itr != itr->state_counts.end()) {
         processed = proc_itr->second;
      }
      
      // Only mark as closed if approved count equals processed count.
      check(approved == processed, "Not all approved items are processed.");

      bounty_tbl.modify(itr, same_payer, [&](auto &row) {
         row.status = "closed"_n;
      });
   }

   // ------------------------------
   // ACTION: cleanup
   // ------------------------------
   // Deletes the bounty record if:
   //  - The bounty status is "closed"
   //  - The total_fungible_assets_deposited vector is empty (i.e. no assets remain deposited)
   ACTION bounties::cleanup(symbol emission_symbol) {
      bounty_table bounty_tbl(get_self(), get_self().value);
      auto itr = bounty_tbl.find(emission_symbol.code().raw());
      check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol.");
      check(itr->status == "closed"_n, "Bounty status must be 'closed' for cleanup.");
      check(itr->total_fungible_assets_deposited.size() == 0, "Deposited assets not zero. Cleanup cannot proceed.");
      deactivate_emission(emission_symbol);
      bounty_tbl.erase(itr);
   }

   ACTION bounties::addactionauth(name org, name action, name authorized_account) {
      require_auth(org);

      actionauths_table _actionauths(get_self(), org.value);
      auto itr = _actionauths.find(action.value);

      if (itr != _actionauths.end()) {
         _actionauths.modify(itr, get_self(), [&](auto& row) {
            auto it = find(row.authorized_accounts.begin(), row.authorized_accounts.end(), authorized_account);
            if (it == row.authorized_accounts.end()) {
               row.authorized_accounts.push_back(authorized_account);
            } else {
               check(false, "Account is already authorized for this action.");
            }
         });
      } else {
         _actionauths.emplace(get_self(), [&](auto& row) {
            row.action = action;
            row.authorized_accounts.push_back(authorized_account);
         });
      }
   }

   ACTION bounties::delactionauth(name org, name action, name authorized_account) {
      require_auth(org);

      actionauths_table _actionauths(get_self(), org.value);
      auto itr = _actionauths.find(action.value);

      if (itr != _actionauths.end()) {
         bool should_erase = false;

         _actionauths.modify(itr, get_self(), [&](auto& row) {
            auto it = find(row.authorized_accounts.begin(), row.authorized_accounts.end(), authorized_account);
            if (it != row.authorized_accounts.end()) {
               row.authorized_accounts.erase(it);
            }
            
            should_erase = row.authorized_accounts.empty();
         });

         if (should_erase) {
            _actionauths.erase(itr);
         }
      }
   }

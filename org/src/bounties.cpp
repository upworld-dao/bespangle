#include <bounties.hpp>
#include <algorithm>
#include <cctype>

ACTION bounties::newbounty(name authorized,
                           
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
                           name open_or_closed_or_external_participation_list)
{

   string action_name = "newbounty";
   string failure_identifier = "CONTRACT: bounties, ACTION: " + action_name + ", MESSAGE: ";
   name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
   check(has_action_authority(org, name(action_name), authorized), "Unauthorized account to execute action");

   // 2. Time validations
   time_point_sec now = current_time_point();
   check(participation_start_time > now, "participation_start_time must be in the future");
   check(participation_end_time > now, "participation_end_time must be in the future");
   check(bounty_settlement_time > now, "bounty_settlement_time must be in the future");
   check(participation_end_time > participation_start_time, "participation_end_time must be greater than participation_start_time");
   check(bounty_settlement_time > participation_end_time, "bounty_settlement_time must be greater than participation_end_time");

   // 3. Validate the enumeration values for the new columns
   check(new_or_old_badge == "new"_n || new_or_old_badge == "old"_n,
         "new_or_old_badge must be either 'new' or 'old'");

   check(limited_or_unlimited_participants == "limited"_n || limited_or_unlimited_participants == "unlimited"_n,
         "limited_or_unlimited_participants must be either 'limited' or 'unlimited'");

   check(open_or_closed_or_external_participation_list == "open"_n ||
             open_or_closed_or_external_participation_list == "closed"_n ||
             open_or_closed_or_external_participation_list == "external"_n,
         "open_or_closed_or_external_participation_list must be 'open', 'closed', or 'external'");
   
   check(max_submissions_per_participant > 0, "max_submissions_per_participant must be greater than 0");

   // 5. Initialize an empty badge_symbol for now
   // This will be set later by either oldbadge or newbadge actions
   symbol badge_symbol;

   // 6. Determine if further actions are needed and set status accordingly
   name status = "init"_n;

   // If we need a badge to be set, or other optional configurations
   if (new_or_old_badge == "new"_n || new_or_old_badge == "old"_n)
   {
      status = "setup"_n; // Badge needs to be set
   }

   if (limited_or_unlimited_participants == "limited"_n)
   {
      status = "setup"_n; // Limited participants count needs to be set
   }

   if (open_or_closed_or_external_participation_list == "closed"_n ||
       open_or_closed_or_external_participation_list == "external"_n)
   {
      status = "setup"_n; // Need to set participants list or external check
   }

   // 7. Insert the new bounty into the bounty table
   bounty_table bounty_tbl(get_self(), get_self().value);

   // Default values for fields that will be set by subsequent actions
   uint64_t max_number_of_participants = 0; // Will be set by limited action if needed

   // Convert emit_assets into contract_assets for storage in the table
   vector<contract_asset> emit_contract_assets;
   for (auto& asset : emit_assets) {
       emit_contract_assets.push_back(contract_asset{
           .contract = name(SIMPLEBADGE_CONTRACT),
           .emit_asset = asset
       });
   }

   bounty_tbl.emplace(get_self(), [&](auto &row)
                      {
        row.emission_symbol = emission_symbol;
        row.badge_symbol = badge_symbol; // Will be updated by oldbadge or newbadge
        row.status = status; // Set to "setup" if further actions needed, "init" otherwise
        row.total_fungible_assets = total_fungible_assets;
        row.max_submissions_per_participant = max_submissions_per_participant;
        row.max_number_of_participants = max_number_of_participants; // Will be updated by limited action if needed
        row.max_fungible_assets_payout_per_winner = max_fungible_assets_payout_per_winner;
        row.participant_check_contract = name(); // Will be updated by external action if needed
        row.participant_check_action = name(); // Will be updated by external action if needed
        row.participant_check_scope = name(); // Will be updated by external action if needed
        row.participants = {}; // Empty map, will be updated by closed action if needed
        row.reviewers = reviewers;
        row.participation_start_time = participation_start_time;
        row.participation_end_time = participation_end_time;
        row.bounty_settlement_time = bounty_settlement_time;
        row.state_counts = {}; // Empty map for state counts
        row.payer = payer;
        
        // Set the new columns
        row.new_or_old_badge = new_or_old_badge;
        row.limited_or_unlimited_participants = limited_or_unlimited_participants;
        row.open_or_closed_or_external_participation_list = open_or_closed_or_external_participation_list;
        
        // Store these for emission creation
        row.bounty_display_name = bounty_display_name;
        row.bounty_ipfs_description = bounty_ipfs_description;
        row.emit_contract_assets = emit_contract_assets;
    });


}

// Private function to check if setup is complete and update status if needed
void bounties::check_and_update_status(bounty& row) {
    if (row.status == "setup"_n) {
        bool setup_complete = true;
        
        // Check if badge is set
        if (row.badge_symbol.code().raw() == 0) {
            setup_complete = false;
        }
        
        // Check if limited participants setup is complete
        if (row.limited_or_unlimited_participants == "limited"_n && row.max_number_of_participants == 0) {
            setup_complete = false;
        }
        
        // Check if closed participants setup is complete
        if (row.open_or_closed_or_external_participation_list == "closed"_n && row.participants.empty()) {
            setup_complete = false;
        }
        
        // Check if external setup is complete
        if (row.open_or_closed_or_external_participation_list == "external"_n && 
            (row.participant_check_contract == name() || row.participant_check_action == name())) {
            setup_complete = false;
        }
        
        // If all setup is complete, update status to init and create the emission
        if (setup_complete) {
            row.status = "init"_n;
            
            // Create the emission
            asset criteria_asset = asset(1, row.badge_symbol);
            vector<asset> emitter_criteria = {criteria_asset};
            bool cyclic = (row.max_submissions_per_participant > 1);
            name org = get_org_from_internal_symbol(row.emission_symbol, "check_and_update_status");
            // Call the newemission action
            action{
                permission_level{get_self(), name("active")},
                name(ANDEMITTER_CONTRACT),
                name("newemission"),
                new_emission_args{
                    .org = org,
                    .emission_symbol = row.emission_symbol,
                    .display_name = row.bounty_display_name,
                    .ipfs_description = row.bounty_ipfs_description,
                    .emitter_criteria = emitter_criteria,
                    .emit_assets = row.emit_contract_assets,
                    .cyclic = cyclic
                }
            }.send();
            
            // Now activate the emission
            action{
                permission_level{get_self(), name("active")},
                name(ANDEMITTER_CONTRACT),
                name("activate"),
                activate_emission_args{
                    .org = org,
                    .emission_symbol = row.emission_symbol
                }
            }.send();
        }
    }
}

ACTION bounties::oldbadge(name authorized, symbol emission_symbol, symbol old_badge_symbol)
{
   // 1. Authorization: Ensure the authorized account is authorized
   string action_name = "oldbadge";
   string failure_identifier = "CONTRACT: bounties, ACTION: " + action_name + ", MESSAGE: ";
   name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
   check(has_action_authority(org, name(action_name), authorized), "Unauthorized account to execute action");


   // 2. Find the bounty entry
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol");


   // 3. Check that new_or_old_badge is set to "old"
   check(itr->new_or_old_badge == "old"_n, "This action can only be executed when new_or_old_badge is set to 'old'");

   // 4. Check that badge_symbol is not already set (should be empty)
   check(itr->badge_symbol.code().raw() == 0, "Badge symbol has already been set");

   // 5. Update the bounty record with the old badge symbol
   bounty_tbl.modify(itr, get_self(), [&](auto &row)
                     {
        row.badge_symbol = old_badge_symbol;
        
        // Check if all required setup is complete and update status if needed
        check_and_update_status(row); });
   vector<name> consumers;

   consumers.push_back(name(ANDEMITTER_CONTRACT));
   for (auto i = 0 ; i < consumers.size(); i++) {
      action {
      permission_level{get_self(), name("active")},
      name(BADGEDATA_CONTRACT),
      name("addfeature"),
      addfeature_args {
         .org = org,
         .badge_symbol = old_badge_symbol,
         .notify_account = consumers[i],
         .memo = ""}
      }.send();
   }
}

ACTION bounties::newbadge(name authorized, symbol emission_symbol, symbol new_badge_symbol,
                          string badge_display_name, string badge_ipfs_image,
                          string badge_description, string badge_creation_memo, bool lifetime_aggregate, bool lifetime_stats)
{
   // 1. Authorization: Ensure the authorized account is authorized
   string action_name = "newbadge";
   string failure_identifier = "CONTRACT: bounties, ACTION: " + action_name + ", MESSAGE: ";
   name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
   check(has_action_authority(org, name(action_name), authorized), "Unauthorized account to execute action");

   // 2. Find the bounty entry
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol");

   // 3. Check that new_or_old_badge is set to "new"
   check(itr->new_or_old_badge == "new"_n, "This action can only be executed when new_or_old_badge is set to 'new'");

   // 4. Check that badge_symbol is not already set (should be empty)
   check(itr->badge_symbol.code().raw() == 0, "Badge symbol has already been set");

   // 5. Validate badge creation fields
   check(!badge_display_name.empty(), "badge_display_name must not be empty for new badge");
   check(!badge_ipfs_image.empty(), "badge_ipfs_image must not be empty for new badge");
   check(!badge_description.empty(), "badge_description must not be empty for new badge");

   // 6. Create the new badge (logic similar to the badge creation in original newbounty)
   // Call the appropriate badge creation functions

   // 7. Update the bounty record with the new badge symbol
   bounty_tbl.modify(itr, get_self(), [&](auto &row){
      row.badge_symbol = new_badge_symbol;
      
      // Check if all required setup is complete and update status if needed
      check_and_update_status(row); });

   action {
      permission_level{get_self(), name("active")},
      name(SIMPLEBADGE_CONTRACT),
      name("create"),
      createsimple_args {
         .org = org,
         .badge_symbol = new_badge_symbol,
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
   consumers.push_back(name(ANDEMITTER_CONTRACT));
   for (auto i = 0 ; i < consumers.size(); i++) {
      action {
      permission_level{get_self(), name("active")},
      name(BADGEDATA_CONTRACT),
      name("addfeature"),
      addfeature_args {
         .org = org,
         .badge_symbol = new_badge_symbol,
         .notify_account = consumers[i],
         .memo = ""}
      }.send();
   }
}

ACTION bounties::limited(name authorized, symbol emission_symbol, uint64_t max_number_of_participants)
{
   // 1. Authorization: Ensure the authorized account is authorized
   string action_name = "limited";
   string failure_identifier = "CONTRACT: bounties, ACTION: " + action_name + ", MESSAGE: ";
   name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
   check(has_action_authority(org, name(action_name), authorized), "Unauthorized account to execute action");

   // 2. Find the bounty entry
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol");

   // 3. Check that limited_or_unlimited_participants is set to "limited"
   check(itr->limited_or_unlimited_participants == "limited"_n,
         "This action can only be executed when limited_or_unlimited_participants is set to 'limited'");

   // 4. Validate max_number_of_participants
   check(max_number_of_participants > 0, "max_number_of_participants must be greater than 0");

   // 5. Update the bounty record with the max number of participants
   bounty_tbl.modify(itr, get_self(), [&](auto &row)
                     {
        row.max_number_of_participants = max_number_of_participants;
        
        // Check if all required setup is complete and update status if needed
        check_and_update_status(row); });
}

ACTION bounties::closed(name authorized, symbol emission_symbol, vector<name> participants)
{
   // 1. Authorization: Ensure the authorized account is authorized
   string action_name = "closed";
   string failure_identifier = "CONTRACT: bounties, ACTION: " + action_name + ", MESSAGE: ";
   name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
   check(has_action_authority(org, name(action_name), authorized), "Unauthorized account to execute action");
   // 2. Find the bounty entry
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol");

   // 3. Check that open_or_closed_or_external_participation_list is set to "closed"
   check(itr->open_or_closed_or_external_participation_list == "closed"_n,
         "This action can only be executed when open_or_closed_or_external_participation_list is set to 'closed'");

   // 4. Validate participants list
   check(!participants.empty(), "participants list cannot be empty");

   // 5. Update the bounty record with the participants list
   map<name, uint64_t> unique_participants;
   for (auto &p : participants)
   {
      unique_participants[p] = 0; // each unique participant appears with a count of 0
   }

   bounty_tbl.modify(itr, get_self(), [&](auto &row)
                     {
        row.participants = unique_participants;
        
        // Check if all required setup is complete and update status if needed
        check_and_update_status(row); });
}

ACTION bounties::external(name authorized, symbol emission_symbol,
                          name participant_check_contract,
                          name participant_check_action,
                          name participant_check_scope)
{
   // 1. Authorization: Ensure the authorized account is authorized
   string action_name = "external";
   string failure_identifier = "CONTRACT: bounties, ACTION: " + action_name + ", MESSAGE: ";
   name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
   check(has_action_authority(org, name(action_name), authorized), "Unauthorized account to execute action");
   // 2. Find the bounty entry
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol");

   // 3. Check that open_or_closed_or_external_participation_list is set to "external"
   check(itr->open_or_closed_or_external_participation_list == "external"_n,
         "This action can only be executed when open_or_closed_or_external_participation_list is set to 'external'");

   // 4. Validate external check parameters
   check(participant_check_contract != name(), "participant_check_contract cannot be empty");
   check(participant_check_action != name(), "participant_check_action cannot be empty");

   // 5. Update the bounty record with the external check parameters
   bounty_tbl.modify(itr, get_self(), [&](auto &row)
                     {
        row.participant_check_contract = participant_check_contract;
        row.participant_check_action = participant_check_action;
        row.participant_check_scope = participant_check_scope;
        
        // Check if all required setup is complete and update status if needed
        check_and_update_status(row); });
}

ACTION bounties::participants(name authorized, symbol emission_symbol, vector<name> participants)
{
   // Ensure the authorized account signs the transaction.
   string action_name = "participants";
   string failure_identifier = "CONTRACT: bounties, ACTION: " + action_name + ", MESSAGE: ";
   name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
   check(has_action_authority(org, name(action_name), authorized), "Unauthorized account to execute action");
   // Open the bounty table.
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol");

   // Only allow adding participants for "closed" participation list
   check(itr->open_or_closed_or_external_participation_list == "closed"_n,
         "cannot add participants when participation list is not closed");

   // Modify the record by adding new participants.
   bounty_tbl.modify(itr, get_self(), [&](auto &row)
                     {
       for ( auto& p : participants ) {
          // If the participant is not already present, add it with a count of 0.
          if ( row.participants.find(p) == row.participants.end() ) {
             row.participants[p] = 0;
          }
       } });
}

ACTION bounties::reviewers(name authorized, symbol emission_symbol, vector<name> reviewers)
{
   // Require that the authorized account signs the transaction.

   string action_name = "reviewers";
   string failure_identifier = "CONTRACT: bounties, ACTION: " + action_name + ", MESSAGE: ";
   name org = get_org_from_internal_symbol(emission_symbol, failure_identifier);
   check(has_action_authority(org, name(action_name), authorized), "Unauthorized account to execute action");  // Open the bounty table.
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol");

   // Modify the bounty record by appending reviewers that are not already in the vector.
   bounty_tbl.modify(itr, get_self(), [&](auto &row)
                     {
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
        } });
}

void bounties::ontransfer(name from, name to, asset amount, string memo)
{
   // Process only incoming transfers to this contract.
   if (to != get_self())
      return;
   // Ignore transfers initiated by this contract.
   if (from == get_self())
      return;

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
   for (auto &ext : bounty_itr->total_fungible_assets)
   {
      if (ext.quantity.symbol == amount.symbol && ext.contract == get_first_receiver())
      {
         asset_found = true;
         target_asset = ext;
         break;
      }
   }
   check(asset_found, "Transferred asset is not accepted by this bounty (not present in total_fungible_assets).");

   // Check if the depositor is the designated payer.
   if (from == bounty_itr->payer)
   {
      // ---------------------------
      // Payer Branch:
      // ---------------------------
      asset current_deposit = asset(0, amount.symbol);
      for (auto &ext : bounty_itr->total_fungible_assets_deposited)
      {
         if (ext.quantity.symbol == amount.symbol && ext.contract == get_first_receiver())
         {
            current_deposit = ext.quantity;
            break;
         }
      }
      asset new_total = current_deposit + amount;
      if (new_total > target_asset.quantity)
      {
         // Refund the entire amount if it would exceed the target.
         action(
             permission_level{get_self(), "active"_n},
             get_first_receiver(),
             "transfer"_n,
             std::make_tuple(get_self(), from, amount, string("Refund: deposit exceeds required amount")))
             .send();
         return;
      }
      // Update the bounty record with the new deposit.
      bounty_tbl.modify(bounty_itr, get_self(), [&](auto &row)
                        {
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
            } });
      // Check whether all target assets have been fully deposited.
      bool all_deposited = true;
      for (auto &total_asset : bounty_itr->total_fungible_assets)
      {
         asset deposited = asset(0, total_asset.quantity.symbol);
         for (auto &ext : bounty_itr->total_fungible_assets_deposited)
         {
            if (ext.quantity.symbol == total_asset.quantity.symbol && ext.contract == total_asset.contract)
            {
               deposited = ext.quantity;
               break;
            }
         }
         if (deposited < total_asset.quantity)
         {
            all_deposited = false;
            break;
         }
      }
      if (all_deposited)
      {
         bounty_tbl.modify(bounty_itr, get_self(), [&](auto &row)
                           { row.status = "deposited"_n; });
      }
   }
   else
   {
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
      if (pool_itr == pool_tbl.end())
      {
         // Create a new pool entry.
         pool_tbl.emplace(get_self(), [&](auto &row)
                          {
                row.account = from;
                extended_asset ext;
                ext.quantity = amount;
                ext.contract = get_first_receiver();
                row.total_fungible_assets_deposited.push_back(ext); });
      }
      else
      {
         // Update the existing pool entry.
         pool_tbl.modify(pool_itr, same_payer, [&](auto &row)
                         {
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
                } });
      }
   }
}

void bounties::status(name requester, uint64_t request_id, name originating_contract, name originating_contract_key, name old_status, name new_status)
{
   // Convert originating_contract_key (expected to be the bounty's emission symbol code as a string)
   // into a symbol with precision 0.
   string sym_str = originating_contract_key.to_string();
   // Convert to uppercase
   transform(sym_str.begin(), sym_str.end(), sym_str.begin(), ::toupper);
   symbol target_sym = symbol(sym_str, 0);
   uint64_t pk = target_sym.code().raw();

   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(pk);
   check(itr != bounty_tbl.end(), "Bounty record not found for the given identifier.");

   bounty_tbl.modify(itr, get_self(), [&](auto &row)
                     {
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
        } });

   if (new_status == "processed"_n)
   {
      distribute(target_sym, requester);
   }
   
   if (new_status == "withdrawn"_n)
   {
      handle_withdrawal(target_sym, requester);
   }
}

void bounties::handle_withdrawal(symbol target_sym, name requester)
{
   // Find the bounty
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto bounty_itr = bounty_tbl.find(target_sym.code().raw());
   
   if (bounty_itr != bounty_tbl.end()) {
      // Modify the bounty to update the submissions map
      bounty_tbl.modify(bounty_itr, same_payer, [&](auto &row) {
         // Find the requester in the submissions map
         auto submission_itr = row.submissions.find(requester);
         
         if (submission_itr != row.submissions.end()) {
            // Subtract one from the submissions count
            if (submission_itr->second > 0) {
               submission_itr->second -= 1;
               
               // If count is now zero, remove the entry
               if (submission_itr->second == 0) {
                  row.submissions.erase(submission_itr);
               }
            }
         }
      });
   }
}

// ------------------------------
// ACTION: signup
// ------------------------------
// Records a participant's sign-up.
ACTION bounties::signup(name participant, symbol emission_symbol, string stream_reason)
{

   require_auth(participant);

   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found.");

   // Check that participation period has started and bounty status is "deposited".
   time_point_sec now = current_time_point();
   check(now >= itr->participation_start_time, "Participation period has not started yet.");
   check(now < itr->participation_end_time, "Participation period has ended.");
   check(itr->status == "deposited"_n, "Bounty is not in 'deposited' status.");

   // Check that the bounty setup is complete by verifying badge_symbol is set
   check(itr->badge_symbol.code().raw() != 0, "Bounty badge is not set yet. Complete the bounty setup first.");

   // Check the participation type based on the new columns
   if (itr->open_or_closed_or_external_participation_list == "closed"_n)
   {
      // For closed participation, the participant must already be present in the allowed list.
      auto p_itr = itr->participants.find(participant);
      check(p_itr != itr->participants.end(), "Participant not in closed participants list.");

      // Update the participant's value to 1 (indicating sign-up).
      if (itr->limited_or_unlimited_participants == "limited"_n)
      {
         check(itr->num_participants + 1 <= itr->max_number_of_participants, "Maximum number of participants exceeded.");
      }

      bounty_tbl.modify(itr, get_self(), [&](auto &row)
                        {
            row.participants[participant] = 1;
            row.num_participants += 1; });
   }
   else if (itr->open_or_closed_or_external_participation_list == "external"_n)
   {
      // For external participation check, call the external contract
      check(itr->participant_check_contract != name(), "External participant check contract not set.");
      check(itr->participant_check_action != name(), "External participant check action not set.");

      action{
          permission_level{get_self(), name("active")},
          itr->participant_check_contract,
          itr->participant_check_action,
          std::make_tuple(itr->participant_check_scope, participant)}
          .send();

      if (itr->limited_or_unlimited_participants == "limited"_n)
      {
         check(itr->num_participants + 1 <= itr->max_number_of_participants, "Maximum number of participants exceeded.");
      }

      bounty_tbl.modify(itr, get_self(), [&](auto &row)
                        {
            row.participants[participant] = 1;
            row.num_participants += 1; });
   }
   else
   {
      // For open participation: if participant is new, add them with value 1.
      if (itr->participants.find(participant) == itr->participants.end())
      {
         if (itr->limited_or_unlimited_participants == "limited"_n)
         {
            check(itr->num_participants + 1 <= itr->max_number_of_participants, "Maximum number of participants exceeded.");
         }

         bounty_tbl.modify(itr, get_self(), [&](auto &row)
                           {
                row.participants[participant] = 1;
                row.num_participants += 1; });
      }
      // If participant already exists, we assume they already signed up.
   }
   // (Optional: Record the stream_reason in an audit log.)
}

// ------------------------------
// ACTION: cancelsignup
// ------------------------------
// Cancels a participant's sign-up if they have not yet submitted.
ACTION bounties::cancelsignup(name participant, symbol emission_symbol, string stream_reason)
{

   require_auth(participant);
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found.");

   // Verify that the participant has signed up.
   auto part_itr = itr->participants.find(participant);
   check(part_itr != itr->participants.end(), "Participant is not signed up.");

   // Ensure that no submission has been recorded.
   auto sub_itr = itr->submissions.find(participant);
   if (sub_itr != itr->submissions.end())
   {
      check(sub_itr->second == 0, "Cannot cancel sign-up after submission has been done.");
   }

   bounty_tbl.modify(itr, get_self(), [&](auto &row)
                     {
        if (row.open_or_closed_or_external_participation_list == "closed"_n) {
            // For closed participation, update the participant's value to 0.
            row.participants[participant] = 0;
            row.num_participants -= 1;
        } else {
            // For open or external participation, erase the participant entry.
            row.participants.erase(participant);

            // Decrement the overall participant count.
            check(row.num_participants > 0, "Number of participants is already zero.");
            row.num_participants -= 1;
        } });

   // (Optional) Record stream_reason in an audit log if desired.
}

// ------------------------------
// ACTION: submit
// ------------------------------
// Invokes the remote "consumesimple" action on the "requests" contract.
// Only allowed if the participant has signed up (their value in participants is 1).
// Then updates the submissions map.
ACTION bounties::submit(name participant, symbol emission_symbol, string stream_reason)
{
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
   bounty_tbl.modify(itr, get_self(), [&](auto &row)
                     {
        uint64_t current_subs = 0;
        auto sub_itr = row.submissions.find(participant);
        if (sub_itr != row.submissions.end()) {
            current_subs = sub_itr->second;
        }
        check(current_subs < row.max_submissions_per_participant, "Maximum submission entries exceeded for this participant.");
        row.submissions[participant] = current_subs + 1; });

   // Prepare remote action parameters.
   // Convert the bounty's emission_symbol.code() to a string, then to a name.
   string symbol_str = itr->emission_symbol.code().to_string();
   std::transform(symbol_str.begin(), symbol_str.end(), symbol_str.begin(), ::tolower);
   name origin_key = name(symbol_str);
   ingestsimple_args ingest_args{
       .originating_contract = get_self(),
       .originating_contract_key = origin_key,
       .requester = participant,
       .approvers = itr->reviewers,
       .to = participant,
       .badge_symbol = itr->badge_symbol,
       .amount = 1,
       .memo = "completed bounty",
       .request_memo = stream_reason,
       .expiration_time = itr->bounty_settlement_time};

   action(
       permission_level{get_self(), "active"_n},
       name(REQUESTS_CONTRACT),
       "ingestsimple"_n,
       ingest_args)
       .send();
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
ACTION bounties::withdraw(name account, symbol emission_symbol)
{

   require_auth(account);
   // Lookup bounty record using emission_symbol as primary key.
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto b_itr = bounty_tbl.find(emission_symbol.code().raw());
   check(b_itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol.");

   if (account == b_itr->payer)
   {
      // Withdrawing by the designated payer.
      // Enforce that the bounty status is closed.
      check(b_itr->status == "closed"_n, "Bounty status must be 'closed' for payer withdrawal.");

      // For each asset in total_fungible_assets_deposited, transfer the entire balance to the payer.
      for (auto const &ext : b_itr->total_fungible_assets_deposited)
      {
         action(
             permission_level{get_self(), "active"_n},
             ext.contract,
             "transfer"_n,
             std::make_tuple(get_self(), account, ext.quantity, string("Payer withdrawal from bounty")))
             .send();
      }
      // Clear the deposits from the bounty record.
      bounty_tbl.modify(b_itr, get_self(), [&](auto &row)
                        { row.total_fungible_assets_deposited.clear(); });
   }
   else
   {
      // Withdrawing by a non-payer account.
      // Use the bountypool table, which is scoped by badge_symbol.
      bountypool_table pool_tbl(get_self(), emission_symbol.code().raw());
      auto pool_itr = pool_tbl.find(account.value);
      check(pool_itr != pool_tbl.end(), "No deposit record found for this account in bountypool.");

      // For each asset deposited by this account, transfer it.
      for (auto const &ext : pool_itr->total_fungible_assets_deposited)
      {
         action(
             permission_level{get_self(), "active"_n},
             ext.contract,
             "transfer"_n,
             std::make_tuple(get_self(), account, ext.quantity, string("Non-payer withdrawal from bounty pool")))
             .send();
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
ACTION bounties::closebounty(symbol emission_symbol)
{
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol.");

   // Retrieve approved and processed counts (defaulting to 0 if not present).
   uint64_t approved = 0;
   uint64_t processed = 0;

   auto appr_itr = itr->state_counts.find("approved"_n);
   if (appr_itr != itr->state_counts.end())
   {
      approved = appr_itr->second;
   }

   auto proc_itr = itr->state_counts.find("processed"_n);
   if (proc_itr != itr->state_counts.end())
   {
      processed = proc_itr->second;
   }

   // Only mark as closed if approved count equals processed count.
   check(approved == processed, "Not all approved items are processed.");

   bounty_tbl.modify(itr, same_payer, [&](auto &row)
                     { row.status = "closed"_n; });
}

// ------------------------------
// ACTION: cleanup
// ------------------------------
// Deletes the bounty record if:
//  - The bounty status is "closed"
//  - The total_fungible_assets_deposited vector is empty (i.e. no assets remain deposited)
ACTION bounties::cleanup(symbol emission_symbol)
{
   bounty_table bounty_tbl(get_self(), get_self().value);
   auto itr = bounty_tbl.find(emission_symbol.code().raw());
   check(itr != bounty_tbl.end(), "Bounty record not found for the given emission_symbol.");
   check(itr->status == "closed"_n, "Bounty status must be 'closed' for cleanup.");
   check(itr->total_fungible_assets_deposited.size() == 0, "Deposited assets not zero. Cleanup cannot proceed.");
   deactivate_emission(emission_symbol);
   bounty_tbl.erase(itr);
}

ACTION bounties::addactionauth(name org, name action, name authorized_account)
{
   require_auth(org);

   actionauths_table _actionauths(get_self(), org.value);
   auto itr = _actionauths.find(action.value);

   if (itr != _actionauths.end())
   {
      _actionauths.modify(itr, get_self(), [&](auto &row)
                          {
            auto it = find(row.authorized_accounts.begin(), row.authorized_accounts.end(), authorized_account);
            if (it == row.authorized_accounts.end()) {
                row.authorized_accounts.push_back(authorized_account);
            } else {
                check(false, "Account is already authorized for this action.");
            } });
   }
   else
   {
      _actionauths.emplace(get_self(), [&](auto &row)
                           {
            row.action = action;
            row.authorized_accounts.push_back(authorized_account); });
   }
}

ACTION bounties::delactionauth(name org, name action, name authorized_account)
{
   require_auth(org);

   actionauths_table _actionauths(get_self(), org.value);
   auto itr = _actionauths.find(action.value);

   if (itr != _actionauths.end())
   {
      bool should_erase = false;

      _actionauths.modify(itr, get_self(), [&](auto &row)
                          {
            auto it = find(row.authorized_accounts.begin(), row.authorized_accounts.end(), authorized_account);
            if (it != row.authorized_accounts.end()) {
                row.authorized_accounts.erase(it);
            }
            
            should_erase = row.authorized_accounts.empty(); });

      if (should_erase)
      {
         _actionauths.erase(itr);
      }
   }
}

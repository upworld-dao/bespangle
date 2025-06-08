#include <mrbadge.hpp>
// todos
// auth contracts
// change duration.
// change supply per duration.
// number of cycles

ACTION mrbadge::starttime(name org, symbol badge_symbol, time_point_sec new_starttime) {
    string action_name = "starttime";
    string failure_identifier = "CONTRACT: mrbadge, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);   

    metadata_table _metadata(_self, org.value);

    // Find the record with the given badge
    auto itr = _metadata.find(badge_symbol.code().raw());
    check(itr != _metadata.end(), failure_identifier + "badge_symbol not found");

    // Check if the current starttime is past the current time
    check(new_starttime >= current_time_point(), failure_identifier + "new starttime in past");
    if (itr->starttime >= current_time_point()) {
        _metadata.modify(itr, get_self(), [&](auto& row) {
            row.starttime = new_starttime;
            row.last_known_cycle_start = new_starttime;
            row.last_known_cycle_end = new_starttime + row.cycle_length - 1;
        });
    } else {
        check(false, failure_identifier + "starttime already past");
    }
}

ACTION mrbadge::cyclelength(name org, symbol badge_symbol, uint64_t new_cycle_length) {
    string action_name = "cyclelength";
    string failure_identifier = "CONTRACT: mrbadge, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);   

    metadata_table _metadata(_self, org.value);

    // Find the record with the given badge
    auto itr = _metadata.find(badge_symbol.code().raw());
    check(itr != _metadata.end(), failure_identifier + "badge_symbol not found");

    // Update the cycle_length
    _metadata.modify(itr, get_self(), [&](auto& row) {
        row.cycle_length = new_cycle_length;
    });
}

ACTION mrbadge::cyclesupply(name org, symbol badge_symbol, uint8_t new_supply_per_cycle) {
    string action_name = "cyclesupply";
    string failure_identifier = "CONTRACT: mrbadge, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);   

    metadata_table _metadata(_self, org.value);

    // Find the record with the given badge
    auto itr = _metadata.find(badge_symbol.code().raw());
    check(itr != _metadata.end(), failure_identifier + "badge_symbol not found");

    // Update cycle supply
    _metadata.modify(itr, get_self(), [&](auto& row) {
        row.supply_per_cycle = new_supply_per_cycle;
    });
}

ACTION mrbadge::create (name org, symbol badge_symbol, 
      time_point_sec starttime, 
      uint64_t cycle_length, 
      uint8_t supply_per_cycle, 
      string display_name, 
      string ipfs_image,
      string description, 
      string memo) {

    string action_name = "create";
    string failure_identifier = "CONTRACT: mrbadge, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);   

    time_point_sec current_time = time_point_sec(current_time_point());
    check(starttime >= current_time, failure_identifier + "mrbadge contract - start time can not be in past");

    metadata_table _metadata(_self, org.value);

    // Todo: Add in all badges
    auto badge_itr = _metadata.find(badge_symbol.code().raw());
    check(badge_itr == _metadata.end(), failure_identifier + "Invalid name or <badge> with this name already exists");

    // Duration of cycle is in seconds
    _metadata.emplace(get_self(), [&](auto& row) {
        row.badge_symbol = badge_symbol;
        row.starttime = starttime;
        row.cycle_length = cycle_length;
        row.last_known_cycle_start = starttime;
        row.last_known_cycle_end = starttime + cycle_length - 1;
        row.supply_per_cycle = supply_per_cycle;
    });

    // Remote orchestrator contract call
    action {
      permission_level{get_self(), name("active")},
      name(BADGEDATA_CONTRACT),
      name("initbadge"),
      initbadge_args {
        .org = org,
        .badge_symbol = badge_symbol,
        .display_name = display_name,
        .ipfs_image = ipfs_image,
        .description = description,
        .memo = memo }
    }.send();


    action {
      permission_level{get_self(), name("active")},
      name(SUBSCRIPTION_CONTRACT),
      name("billing"),
      billing_args {
        .org = org,
        .actions_used = 1}
    }.send();

}


    ACTION mrbadge::issue (name org, asset badge_asset,
      name from, 
      name to,
      string memo) {
    string action_name = "issue";
    string failure_identifier = "CONTRACT: mrbadge, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);   

    check(from != to, "can not be same - from, to");

    metadata_table _metadata (_self, org.value);
    auto badge_itr = _metadata.require_find(badge_asset.symbol.code().raw(), "Not a valid mr badge");
    time_point_sec last_known_cycle_start = badge_itr->last_known_cycle_start;
    time_point_sec last_known_cycle_end = badge_itr->last_known_cycle_end;
    uint64_t cycle_length = badge_itr->cycle_length;
    uint8_t supply_per_cycle = badge_itr->supply_per_cycle;
    time_point_sec starttime = badge_itr->starttime;
    time_point_sec current_time = time_point_sec(current_time_point());
    check(current_time >= starttime, "can not give this badge yet " + to_string(current_time.sec_since_epoch()));
    //check(false, "reached here");

    // find current cycle start and end.

    bool current_cycle_found = last_known_cycle_start <= current_time && last_known_cycle_end >= current_time;
    bool cycle_update_needed = false;

    while(!current_cycle_found) {
      cycle_update_needed = true; // optimize this.
      last_known_cycle_start = last_known_cycle_start + cycle_length;
      last_known_cycle_end = last_known_cycle_end + cycle_length;
      current_cycle_found = last_known_cycle_start <= current_time && last_known_cycle_end >= current_time;
    }
    //// update current cycle in local metadata
    if(cycle_update_needed) {
      _metadata.modify(badge_itr, get_self(), [&](auto& row) {
        row.last_known_cycle_end = last_known_cycle_end;
        row.last_known_cycle_start = last_known_cycle_start;
      }); 
    } 

    time_point_sec current_cycle_start = last_known_cycle_start;
    time_point_sec current_cycle_end = last_known_cycle_end;


    // check and update balance in local stats

    stats_table _stats (_self, from.value);
    auto stats_itr = _stats.find(badge_asset.symbol.code().raw());

    if(stats_itr == _stats.end()) {
      check(badge_asset.amount <= supply_per_cycle, failure_identifier + "<amount> exceeds available <supply_per_cycle>. Can only issue <supply_per_cycle> <badge>");
      _stats.emplace(get_self(), [&](auto& row) {
        row.badge_asset = badge_asset;
        row.org = org;
        row.last_claimed_time = current_time;
      });
    } else if (current_cycle_start <= stats_itr->last_claimed_time) {
      uint64_t new_amount = stats_itr->badge_asset.amount + badge_asset.amount;
      check(new_amount <= supply_per_cycle, "<balance + amount> exceeds available <supply_per_cycle>. Can only issue <supply_per_cycle - balance> <badge>");
      _stats.modify(stats_itr, get_self(), [&](auto& row) {
        row.badge_asset = row.badge_asset + badge_asset;
        row.last_claimed_time = current_time;
      });
    } else {
      check(badge_asset.amount <= supply_per_cycle, "<amount> exceeds available <supply_per_cycle>. Can only issue <supply_per_cycle - balance> <badge>");
      _stats.modify(stats_itr, get_self(), [&](auto& row) {
        row.badge_asset = badge_asset;
        row.last_claimed_time = current_time;
      });
    }

    //  remote balance update and badge issuance
    action {
      permission_level{get_self(), name("active")},
      name(BADGEDATA_CONTRACT),
      name("achievement"),
      achievement_args {
        .org = org,
        .badge_asset = badge_asset,
        .from = from,
        .to = to,
        .memo = memo }
    }.send();


  }

 

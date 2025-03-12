#include <simplebadge.hpp>

    // todo 
    // 1) check for cycles to throw better error message, c.
    // 2) replace values in error messages.
    // 3) put profiles contract name in a global constant
    // 4) add action to update image json.
    // 5) add action to update details json.

  ACTION simplebadge::create (
      name org,
      symbol badge_symbol, 
      string display_name, 
      string ipfs_image,
      string description, 
      string memo) {

    string action_name = "create";
    string failure_identifier = "CONTRACT: simplebadge, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

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

  ACTION simplebadge::issue (name org, asset badge_asset, name to, string memo) {
    string action_name = "issue";
    string failure_identifier = "CONTRACT: simplebadge, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);
    
    action {
      permission_level{get_self(), name("active")},
      name(BADGEDATA_CONTRACT),
      name("achievement"),
      achievement_args {
        .org = org,
        .badge_asset = badge_asset,
        .from = get_self(),
        .to = to,
        .memo = memo }
    }.send();
  }

    

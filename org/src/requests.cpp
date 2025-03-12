#include <requests.hpp>

ACTION requests::ingestsimple(
    name originating_contract,
    name originating_contract_key,
    name requester, 
    vector<name> approvers, 
    name to, 
    symbol badge_symbol,
    uint64_t amount, 
    string memo,
    string request_memo,
    time_point_sec expiration_time) {
    
    string action_name = "ingestsimple";
    string failure_identifier = "CONTRACT: requests, ACTION: " + action_name + ", MESSAGE: ";
    check_internal_auth(get_self(), name(action_name), failure_identifier);

    check(current_time_point().sec_since_epoch() < expiration_time.sec_since_epoch(), "Request expiration time must be in the future");

    // Extract org from badge_symbol
    name org = get_org_from_internal_symbol(badge_symbol, failure_identifier);

    // Scope by org
    sequence_index seq_table(get_self(), org.value);
    auto seq_itr = seq_table.find(name("request").value);
    uint64_t request_id;

    if (seq_itr == seq_table.end()) {
        seq_table.emplace(get_self(), [&](auto& row) {
            row.key = name("request");
            row.seq_id = 1;
        });
        request_id = 1;
    } else {
        // First get the current value
        request_id = seq_itr->seq_id + 1;
        // Then increment the counter in a separate step
        seq_table.modify(seq_itr, same_payer, [&](auto& row) {
            row.seq_id = request_id;
        });
    }

    // Initialize approver status
    vector<pair<name, name>> approver_statuses;
    for (const auto& approver : approvers) {
        approver_statuses.push_back({approver, "pending"_n});
    }

    // Store in request table
    request_index req_table(get_self(), org.value);
    req_table.emplace(get_self(), [&](auto& row) {
        row.request_id = request_id;
        row.action = "issuesimple"_n;
        row.requester = requester;
        row.originating_contract = originating_contract;
        row.originating_contract_key = originating_contract_key;
        row.approvers = approver_statuses;
        row.status = "pending"_n;
        row.expiration_time = expiration_time;
    });

    // Store in simissue table
    simissue_index sim_table(get_self(), org.value);
    sim_table.emplace(get_self(), [&](auto& row) {
        row.request_id = request_id;
        row.to = to;
        row.badge_symbol = badge_symbol;
        row.amount = amount;
        row.memo = memo;
    });

    action(
        permission_level{get_self(), "active"_n},
        get_self(),
        "sharestatus"_n,
        sharestatus_args {
            .requester = requester,
            .request_id = request_id,
            .originating_contract = originating_contract,
            .originating_contract_key = originating_contract_key,
            .old_status = "blank"_n,
            .new_status = "pending"_n }
    ).send();
}

ACTION requests::initseq(name org, name key, uint64_t seq_id) {
    require_auth(get_self());
    string action_name = "initseq";
    string failure_identifier = "CONTRACT: requests, ACTION: " + action_name + ", MESSAGE: ";
    //check_internal_auth(get_self(), name(action_name), failure_identifier);
    
    // Check if a sequence with this key already exists
    sequence_index seq_table(get_self(), org.value);
    auto seq_itr = seq_table.find(key.value);
    
    if (seq_itr == seq_table.end()) {
        // Create new sequence entry
        seq_table.emplace(get_self(), [&](auto& row) {
            row.key = key;
            row.seq_id = seq_id;
        });
    } else {
        // Update existing sequence entry
        seq_table.modify(seq_itr, same_payer, [&](auto& row) {
            row.seq_id = seq_id;
        });
    }
}

ACTION requests::processone(name org, uint64_t request_id) {
    
    request_index req_table(get_self(), org.value);
    auto req_itr = req_table.find(request_id);
    check(req_itr != req_table.end(), "Request ID not found");
    
    simissue_index sim_table(get_self(), org.value);
    if (current_time_point().sec_since_epoch() >= req_itr->expiration_time.sec_since_epoch()) {
        auto sim_itr = sim_table.find(request_id);
        check(sim_itr != sim_table.end(), "Matching simissue entry not found");                
        
        if (req_itr->status == "approved"_n) {
            action(
                permission_level{get_self(), "active"_n},
                name(SIMPLEBADGE_CONTRACT),
                "issue"_n,
                simpleissue_args {
                    .org = org,
                    .badge_asset = asset(sim_itr->amount, sim_itr->badge_symbol),
                    .to = sim_itr->to,
                    .memo = sim_itr->memo
                }
            ).send();

            action(
                permission_level{get_self(), "active"_n},
                get_self(),
                "sharestatus"_n,
                sharestatus_args {
                    .requester = req_itr->requester,
                    .request_id = req_itr->request_id,
                    .originating_contract = req_itr->originating_contract,
                    .originating_contract_key = req_itr->originating_contract_key,
                    .old_status = "approved"_n,
                    .new_status = "processed"_n }
            ).send();
        } 
        req_table.erase(req_itr);
        sim_table.erase(sim_itr);
    } else {
        check(false, "not time yet to process this request_id");
    }
    
}

ACTION requests::process(name org, uint16_t batch_size) {
    
    request_index req_table(get_self(), org.value);
    auto req_itr = req_table.begin();
    uint16_t processed = 0;
    
    simissue_index sim_table(get_self(), org.value);
    
    while (req_itr != req_table.end() && processed < batch_size) {
        uint64_t request_id = req_itr->request_id;
        if (current_time_point().sec_since_epoch() >= req_itr->expiration_time.sec_since_epoch()) {
            auto sim_itr = sim_table.find(request_id);
            check(sim_itr != sim_table.end(), "Matching simissue entry not found");                
            
            if (req_itr->status == "approved"_n) {
                action(
                    permission_level{get_self(), "active"_n},
                    name(SIMPLEBADGE_CONTRACT),
                    "issue"_n,
                    simpleissue_args {
                        .org = org,
                        .badge_asset = asset(sim_itr->amount, sim_itr->badge_symbol),
                        .to = sim_itr->to,
                        .memo = sim_itr->memo
                    }
                ).send();

                action(
                    permission_level{get_self(), "active"_n},
                    get_self(),
                    "sharestatus"_n,
                    sharestatus_args {
                        .requester = req_itr->requester,
                        .request_id = req_itr->request_id,
                        .originating_contract = req_itr->originating_contract,
                        .originating_contract_key = req_itr->originating_contract_key,
                        .old_status = "approved"_n,
                        .new_status = "processed"_n }
                ).send();
            } 

            sim_table.erase(sim_itr);
            req_itr = req_table.erase(req_itr);
            processed++;

        } else {
            ++req_itr;
        }
    }
}

ACTION requests::evidence(name authorized, name org, uint64_t request_id, string stream_reason) {
    require_auth(authorized);
    
    request_index req_table(get_self(), org.value);
    auto req_itr = req_table.find(request_id);
    check(req_itr != req_table.end(), "Request ID not found");
    check(current_time_point().sec_since_epoch() < req_itr->expiration_time.sec_since_epoch(), "Cannot modify expired request");

    check(authorized == req_itr->requester, "not authorized to submit evidence for " + req_itr->requester.to_string());

    // Reset approver statuses to pending
    vector<pair<name, name>> reset_approvers = req_itr->approvers;
    for (auto& entry : reset_approvers) {
        entry.second = "pending"_n;
    }
    name old_status = req_itr->status;
    // Modify request status and reset approvers
    req_table.modify(req_itr, same_payer, [&](auto& row) {
        row.approvers = reset_approvers;
        row.status = "pending"_n;
    });
    
    // Notify via sharestatus action
    action(
        permission_level{get_self(), "active"_n},
        get_self(),
        "sharestatus"_n,
        sharestatus_args {
            .requester = req_itr->requester,
            .request_id = req_itr->request_id,
            .originating_contract = req_itr->originating_contract,
            .originating_contract_key = req_itr->originating_contract_key,
            .old_status = old_status,
            .new_status = "pending"_n }
    ).send();
}

ACTION requests::withdraw(name authorized, name org, uint64_t request_id, string stream_reason) {
    require_auth(authorized);
    
    request_index req_table(get_self(), org.value);
    auto req_itr = req_table.find(request_id);
    check(req_itr != req_table.end(), "Request ID not found");
    check(current_time_point().sec_since_epoch() < req_itr->expiration_time.sec_since_epoch(), "Cannot withdraw expired request");

    check(authorized == req_itr->requester, "not authorized to withdraw requests for " + req_itr->requester.to_string());
    name old_status = req_itr->status;  
    
    simissue_index sim_table(get_self(), org.value);
    auto sim_itr = sim_table.find(request_id);
  
    // Erase from request table
    req_table.erase(req_itr);
    
    // Erase from simissue table if exists
    if (sim_itr != sim_table.end()) {
        sim_table.erase(sim_itr);
    }
    
    // Notify via sharestatus action
    action(
        permission_level{get_self(), "active"_n},
        get_self(),
        "sharestatus"_n,
        sharestatus_args {
            .requester = req_itr->requester,
            .request_id = req_itr->request_id,
            .originating_contract = req_itr->originating_contract,
            .originating_contract_key = req_itr->originating_contract_key,
            .old_status = old_status,
            .new_status = "withdrawn"_n }
    ).send();
}

ACTION requests::sharestatus(name requester, uint64_t request_id, name originating_contract, name originating_contract_key, name old_status, name new_status) {
    require_recipient(originating_contract);
}

ACTION requests::approve(name approver, name org, uint64_t request_id, string stream_reason) {
    require_auth(approver);

    request_index req_table(get_self(), org.value);
    auto req_itr = req_table.find(request_id);
    check(req_itr != req_table.end(), "Request ID not found");
    check(current_time_point().sec_since_epoch() < req_itr->expiration_time.sec_since_epoch(), "Cannot approve expired request");
  
    // Update approver status
    bool updated = false;
    vector<pair<name, name>> updated_approvers = req_itr->approvers;
    for (auto& entry : updated_approvers) {
        if (entry.first == approver && entry.second == "pending"_n) {
            entry.second = "approved"_n;
            updated = true;
            break;
        }
    }
    check(updated, "Approver not found or already approved");

    // Check if majority have approved
    int approved_count = 0;
    for (const auto& entry : updated_approvers) {
        if (entry.second == "approved"_n) {
            approved_count++;
        }
    }
    bool majority_approved = approved_count > (updated_approvers.size() / 2);
    name old_status = req_itr->status;
    // Modify request status if necessary
    req_table.modify(req_itr, same_payer, [&](auto& row) {
        row.approvers = updated_approvers;
        if (majority_approved) {
            row.status = "approved"_n;
        }
    });

    // Notify via sharestatus action if status changes
    if (majority_approved) {
        // Notify via sharestatus action
        action(
            permission_level{get_self(), "active"_n},
            get_self(),
            "sharestatus"_n,
            sharestatus_args {
                .requester = req_itr->requester,
                .request_id = req_itr->request_id,
                .originating_contract = req_itr->originating_contract,
                .originating_contract_key = req_itr->originating_contract_key,
                .old_status = old_status,
                .new_status = "approved"_n }
        ).send();
    }
}

ACTION requests::reject(name approver, name org, uint64_t request_id, string stream_reason) {
    require_auth(approver);

    request_index req_table(get_self(), org.value);
    auto req_itr = req_table.find(request_id);
    check(req_itr != req_table.end(), "Request ID not found");
    check(current_time_point().sec_since_epoch() < req_itr->expiration_time.sec_since_epoch(), "Cannot reject expired request");

    // Update approver status
    bool updated = false;
    vector<pair<name, name>> updated_approvers = req_itr->approvers;
    for (auto& entry : updated_approvers) {
        if (entry.first == approver && entry.second == "pending"_n) {
            entry.second = "rejected"_n;
            updated = true;
            break;
        }
    }
    check(updated, "Approver not found or already rejected");

    // Check if majority have rejected
    int rejected_count = 0;
    for (const auto& entry : updated_approvers) {
        if (entry.second == "rejected"_n) {
            rejected_count++;
        }
    }
    bool majority_rejected = rejected_count > (updated_approvers.size() / 2);
    name old_status = req_itr->status;
    // Modify request status if necessary
    req_table.modify(req_itr, same_payer, [&](auto& row) {
        row.approvers = updated_approvers;
        if (majority_rejected) {
            row.status = "rejected"_n;
        }
    });

    // Notify via sharestatus action if status changes
    if (majority_rejected) {
        action(
            permission_level{get_self(), "active"_n},
            get_self(),
            "sharestatus"_n,
            sharestatus_args {
                .requester = req_itr->requester,
                .request_id = req_itr->request_id,
                .originating_contract = req_itr->originating_contract,
                .originating_contract_key = req_itr->originating_contract_key,
                .old_status = old_status,
                .new_status = "rejected"_n }
        ).send();
    }
}


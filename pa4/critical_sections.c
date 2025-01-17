#include "critical_sections.h"

#include "banking.h"
#include "ipc.h"
#include "ipc_message.h"
#include "lamport_time.h"
#include "process_pub.h"

#include <assert.h>
#include <stdio.h>


bool is_proc_allowed_cs(struct process* self) {
    if (token_list_is_empty()) {
        return false;
    }
    token_lamport min_token = token_list_min();
    return min_token.proc_id == get_pr_id(self) 
        && self -> state.reply_msg_received == self -> children_amount - 1;
}


int process_request_cs(struct process* self) {
    Message* msg = NULL;
    token_lamport token = { .proc_id = get_pr_id(self), .timestamp = get_lamport_time() };

    token_list_append(token);
    if (create_message(CS_REQUEST, 0, NULL, get_lamport_time(), &msg) != 0) {
        return 1;
    }
    if (send_multicast(self, msg) != 0) {
        return 2;
    }
    free_message(&msg);
    return 0;
}

int process_release_cs(struct process* self) {
    Message* msg = NULL;
    if (!token_list_is_empty()) {
        token_list_remove_min();
    }
    self -> state.reply_msg_received = 0;
    self -> state.is_allowed_cs = false;

    if (create_message(CS_RELEASE, 0, NULL, get_lamport_time(), &msg) != 0) {
        return 1;
    }
    if (send_multicast(self, msg) != 0) {
        return 2;
    }
    free_message(&msg);
    return 0;
}

int process_reply_cs(struct process* self, const local_id dst) {
    Message* msg = NULL;
    if (create_message(CS_REPLY, 0, NULL, get_lamport_time(), &msg) != 0) {
        return 1;
    }
    if (send(self, dst, msg) != 0) {
        // debug
        fprintf(stderr, "%d: proc %d WHILE loop needed??", get_lamport_time(), get_pr_id(self));
        return 2;
    }
    free_message(&msg);
    return 0;
}



void handle_cs_request(struct process* self, Message* msg, local_id from) {
    token_lamport received_token = {.proc_id = from, .timestamp = msg->s_header.s_local_time};
    token_list_append(received_token);

    // token_lamport added = token_list_last();
    // fprintf(stdout, "proc %d: Token added : id %d time %d\n", get_pr_id(self), added.proc_id, added.timestamp);

    if (process_reply_cs(self, from) != 0) {
        fprintf(stderr, "%d: proc %d fialed to reply CS REQUEST to %d", get_lamport_time(), get_pr_id(self), from);
    }
}

void handle_cs_reply(struct process* self, Message* msg, local_id from) {
    self -> state.reply_msg_received += 1;
    if (is_proc_allowed_cs(self)) {
        self -> state.is_allowed_cs = true;
    }
}

void handle_cs_release(struct process* self, Message* msg, local_id from) {
    if (!token_list_is_empty()) {
        token_list_remove_min();
        if (is_proc_allowed_cs(self)) {
            self -> state.is_allowed_cs = true;
        }
    }
}


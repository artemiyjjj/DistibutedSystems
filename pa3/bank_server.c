#include "bank_server.h"
#include "banking.h"
#include "chanel.h"
#include "ipc.h"
#include "ipc_message.h"
#include "pa2345.h"
#include "process_pub.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local to each process state of logical time
static timestamp_t local_lamport_time = 0;


/**
 * @brief Transfer handling logic on bank server side
 * 
 * @param self 
 * @param msg 
 * @return int 0 - Success
 * @return int -1 - Process is not allowed to perform transfers
 * @return int 1 - Provided structures are not valid
 * @return int 2 - Transfer message has no relation to this process
 * @return int 3 - Failed to send message
 */
int handle_transfer(struct process* self, Message* msg) {
    TransferOrder* order = (TransferOrder*) msg -> s_payload;
    Message* send_msg = NULL;

    if (self == NULL || msg == NULL) {
        return 1;
    }

    if (order -> s_src == self -> id) {
        // internal event
        self -> local_balance -= order -> s_amount;

        set_lamport_time(NULL);
        msg -> s_header.s_local_time = get_lamport_time();
        if (send(self, order -> s_dst, msg) != 0) {
            self -> local_balance += order -> s_amount;
            return 3;
        }
        fprintf(log_events_stream, log_transfer_out_fmt, get_lamport_time(), order -> s_src, order -> s_amount, order -> s_dst);
        fprintf(stdout, log_transfer_out_fmt, get_lamport_time(), order -> s_src, order -> s_amount, order -> s_dst);
    }
    else if (order -> s_dst == self -> id) {
        // internal event
        self -> local_balance += order -> s_amount;

        fprintf(log_events_stream, log_transfer_in_fmt, get_lamport_time(), order -> s_dst, order -> s_amount, order -> s_src);
        fprintf(stdout, log_transfer_in_fmt, get_lamport_time(), order -> s_dst, order -> s_amount, order -> s_src);

        append_balance_history(self);
        append_pending_balance_history(self, msg);

        set_lamport_time(NULL);
        if (create_message(ACK, 0, NULL, get_lamport_time(), &send_msg) != 0) {
            self -> local_balance -= order -> s_amount;
            return 3;
        }
        if (send(self, PARENT_ID, send_msg) != 0) {
            self -> local_balance -= order -> s_amount;
            return 3;
        }
    }
    else {
        return 2;
    }
    append_balance_history(self);
    free_message(&send_msg);
    fflush(stdout);
    return 0;
}

int handle_stop(struct process* self, Message* msg) {
    int ret_val = 0;
    Message* send_msg = NULL;
    char* msg_contents = malloc(strlen(log_done_fmt)*2);
    if (msg_contents == NULL) {
        return 1;
    }
    sprintf(msg_contents, log_done_fmt, get_lamport_time(), self -> id, self -> local_balance);
    fprintf(stdout, "%s", msg_contents);
    fprintf(log_events_stream, "%s", msg_contents);

    set_lamport_time(NULL);
    append_balance_history(self);

    if (create_message(DONE, strlen(msg_contents), msg_contents, get_lamport_time(), &send_msg) != 0) {
        free(msg_contents);
        return 1;
    }
    // send done msg
    if (send_multicast(self, send_msg) != 0) {
        ret_val = 2;
    }
    free(msg_contents);
    free_message(&send_msg);
    return ret_val;
}

void append_pending_balance_history(struct process* self, Message* transfer_message) {
    if (transfer_message == NULL) {
        return;
    }
    assert(transfer_message -> s_header.s_type == TRANSFER);
    TransferOrder* order = (TransferOrder*) transfer_message -> s_payload;
    if (order -> s_dst != get_pr_id(self)) {
        return;
    }

    // append pending $ based on knowledge of time transfer was initiated
    timestamp_t transction_start = transfer_message -> s_header.s_local_time;
    for (timestamp_t i = transction_start; i < get_lamport_time(); i++) {
        self -> balanceHistory -> s_history[i].s_balance_pending_in += order -> s_amount;
    }
}

/**
 * @brief Append new element of BalanceState to BalanceHistory at index equal
 * `get_lamport_time()`.
 * 
 * 
 * @param self 
 */
void append_balance_history(struct process* self) {
    BalanceState new_balance_state = {
        .s_balance = self -> local_balance,
        .s_balance_pending_in = 0,
        .s_time = get_lamport_time()
    };

    if (self -> balanceHistory -> s_history_len == 0) {
        self -> balanceHistory -> s_history[0] = new_balance_state;
        self -> balanceHistory -> s_history_len += 1;
        return;
    }

    // arr index == time 
    // last index == s_hist_len - 1
    BalanceState prev_state = self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1];

    // fill gaps
    while (new_balance_state.s_time > self -> balanceHistory -> s_history_len) {
        prev_state.s_time++;
        self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len] = prev_state;
        self -> balanceHistory -> s_history_len += 1;
        // fprintf(stdout, "%hu DEB fill: pr %hi, time %hi, money %hi\n", get_physical_time(),
        //                         self -> id, 
        //                         self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1] . s_time,
        //                         self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1] . s_balance
        //         );
    } 


    // self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len] = new_balance_state;
    // self -> balanceHistory -> s_history_len += 1;
    self -> balanceHistory -> s_history[new_balance_state.s_time] = new_balance_state;
    self -> balanceHistory -> s_history_len = 
                new_balance_state.s_time == self -> balanceHistory -> s_history_len
                ? self -> balanceHistory -> s_history_len + 1
                : self -> balanceHistory -> s_history_len;
                                              

    // fprintf(stdout, "%hu DEB add: pr %hi, time %hi, money %hi\n", get_physical_time(),
    //                              self -> id,
    //                              self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1] . s_time,
    //                              self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1] . s_balance
    //                 );
}

timestamp_t get_lamport_time(void) {
    return local_lamport_time;
}

/**
 * @brief Sync clock if msg provided and increment the lamport clock
 * 
 * @param self 
 * @param received_msg struct Message entity for receiveing event, NULL if inner or sending event
 */
void set_lamport_time(Message *received_msg) {
    timestamp_t cur_time = get_lamport_time();
    timestamp_t recieved_time = 0;

    local_lamport_time += 1;
    if (received_msg == NULL) {
        return;
    } else {
        recieved_time = received_msg -> s_header.s_local_time;
    }

    if (recieved_time >= cur_time) {
        local_lamport_time = recieved_time;
        local_lamport_time += 1;
    }
}

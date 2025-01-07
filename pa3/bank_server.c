#include "bank_server.h"
#include "banking.h"
#include "chanel.h"
#include "ipc.h"
#include "ipc_message.h"
#include "pa2345.h"
#include "process_pub.h"

#include <assert.h>
#include <stdio.h>

/**
 * @brief Transfer handling logic on bank server side
 * 
 * @param self 
 * @param msg 
 * @return int 0 - Success
 * @return int 1 - Provided structures are not valid
 * @return int 2 - Transfer message has no relation to this process
 * @return int 3 - Failed to send message
 */
int handle_transfer(void* self, Message* msg) {
    struct process* pr = (struct process*) self;
    TransferOrder* order = (TransferOrder*) msg -> s_payload;
    Message* send_msg = NULL;

    if (self == NULL || msg == NULL) {
        return 1;
    }

    set_lamport_time(NULL);
    if (order -> s_src == pr -> id) {
        pr -> local_balance -= order -> s_amount;
        pr -> pending_balance = order -> s_amount;
        if (send(pr, order -> s_dst, msg) != 0) {
            pr -> local_balance += order -> s_amount;
            return 3;
        }
        fprintf(log_events_stream, log_transfer_out_fmt, pr -> local_time, order -> s_src, order -> s_amount, order -> s_dst);
        fprintf(stdout, log_transfer_out_fmt, pr -> local_time, order -> s_src, order -> s_amount, order -> s_dst);
        fflush(stdout);
    }
    else if (order -> s_dst == pr -> id) {
        pr -> local_balance += order -> s_amount;
        if (create_message(ACK, 0, NULL, pr -> local_time, &send_msg) != 0) {
            pr -> local_balance -= order -> s_amount;
            return 3;
        }
        if (send(pr, PARENT_ID, send_msg) != 0) {
            pr -> local_balance -= order -> s_amount;
            return 3;
        }
        free_message(&send_msg);
        fprintf(log_events_stream, log_transfer_in_fmt, pr -> local_time, order -> s_dst, order -> s_amount, order -> s_src);
        fprintf(stdout, log_transfer_in_fmt, pr -> local_time, order -> s_dst, order -> s_amount, order -> s_src);
        fflush(stdout);
    }
    else {
        return 2;
    }
    return 0;
}

void append_balance_history(struct process* self) {
    BalanceState new_balance_state = {
        .s_balance = self -> local_balance,
        .s_balance_pending_in = self -> pending_balance,
        .s_time = self -> local_time
    };


    if (self -> balanceHistory -> s_history_len == 0) {
        self -> balanceHistory -> s_history[0] = new_balance_state;
        self -> balanceHistory -> s_history_len++;
        return;
    }
    // arr index == time 
    // last index == s_hist_len - 1
    BalanceState prev_state = self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1];

    // fill gaps
    while (new_balance_state.s_time > self -> balanceHistory -> s_history_len) {
        prev_state.s_time++;
        self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len] = prev_state;
        self -> balanceHistory -> s_history_len++;
        // fprintf(stdout, "%hu DEB fill: pr %hi, time %hi, money %hi\n", get_physical_time(),
        //                         self -> id, 
        //                         self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1] . s_time,
        //                         self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1] . s_balance
        //         );
    } 

    self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len] = new_balance_state;
    self -> balanceHistory -> s_history_len++;

    // fprintf(stdout, "%hu DEB add: pr %hi, time %hi, money %hi\n", get_physical_time(),
    //                              self -> id,
    //                              self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1] . s_time,
    //                              self -> balanceHistory -> s_history[self -> balanceHistory -> s_history_len - 1] . s_balance
    //                 );
}

timestamp_t get_lamport_time(void) {
    return lamport_proc -> local_time;
}

/**
 * @brief Set the lamport time object
 * 
 * @param self 
 * @param received_msg struct Message entity for receiveing event, NULL if inner or sending event
 */
void set_lamport_time(Message *received_msg) {
    timestamp_t cur_time = lamport_proc -> local_time;
    timestamp_t recieved_time = 0;

    lamport_proc -> local_time++;

    if (received_msg == NULL) {
        return;
    } else {
        recieved_time = received_msg -> s_header.s_local_time;
    }

    if (recieved_time > cur_time) {
        lamport_proc -> local_time = ++recieved_time;
    }
}

#include "banking.h"
#include "ipc.h"
#include "ipc_message.h"
#include "pa2345.h"
#include "process_pub.h"

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

    if (order -> s_src == pr -> id) {
        pr -> local_balance -= order -> s_amount;
        if (send(pr, order -> s_dst, msg) != 0) {
            pr -> local_balance += order -> s_amount;
            return 3;
        }
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
        fprintf(stdout, log_transfer_in_fmt, pr -> local_time, order -> s_dst, order -> s_amount, order -> s_src);
        fflush(stdout);
    }
    else {
        return 2;
    }
    return 0;
}


#include "banking.h"

#include "ipc.h"
#include "ipc_message.h"
#include "process_pub.h"

#include <stdio.h>
#include <stdlib.h>

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount)
{
    // student, please implement me
    // ok, computer
    int receive_res = -1;
    struct process*     pr = (struct process*) parent_data;
    Message*            msg = NULL;
    TransferOrder       msg_contents = {
        .s_src = src,
        .s_dst = dst,
        .s_amount = amount
    };

    if (create_message(TRANSFER, sizeof(TransferOrder),
                    &msg_contents, pr -> local_time, &msg) != 0) {
        return;
    }
    if (send(pr, src, msg) != 0) {
        return;
    }
    free_message(&msg);
    /// In case of completely async bank client behaviour
    // pr -> waiting_acks_amount++;
    // pr -> waiting_pr_ids = realloc(pr -> waiting_pr_ids, sizeof(balance_t) * pr -> waiting_acks_amount);
    // if (pr -> waiting_pr_ids == NULL) {
    //     fprintf(stderr, "Proc %d: failed to allocate memory", pr -> id);
    //     return;
    // }
    // pr -> waiting_pr_ids[pr -> waiting_acks_amount - 1] = dst;
    /// According to assignment: wait for an ACK
    if (create_empty_msg(&msg) != 0) {
        return;
    }
    do {
        receive_res = receive(pr, dst, msg);
        if (receive_res != 0 && receive_res != 1) {
            fprintf(stderr, "Pr %d: Failed to get ACK from transfer, ret code %d\n", pr -> id, receive_res);
            fflush(stderr);
            free_message(&msg);
            return;
        }
    }
    while (receive_res != 0 && msg -> s_header.s_type != ACK);
    fprintf(stdout, "%d: Process %d: received ACK from process %d\n", get_physical_time(), get_pr_id(pr), dst);
    fflush(stdout);
    free_message(&msg);
    return;
}

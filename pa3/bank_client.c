#include "bank_server.h"
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
    struct process*     pr = (struct process*) parent_data;
    Message*            msg = NULL;
    TransferOrder       msg_contents = {
        .s_src = src,
        .s_dst = dst,
        .s_amount = amount
    };
    int receive_res = -1;

    set_lamport_time(NULL);
    if (create_message(TRANSFER, sizeof(TransferOrder),
                    &msg_contents, get_lamport_time(), &msg) != 0) {
        return;
    }
    if (send(pr, src, msg) != 0) {
        return;
    }
    fprintf(stdout, "%d: Process %d send transfter request $%d from %d to %d\n", get_lamport_time(), PARENT_ID, amount, src, dst);
    free_message(&msg);

    /// According to assignment: block and wait for an ACK
    if (create_empty_msg(&msg) != 0) {
        return;
    }
    do {
        receive_res = receive(pr, dst, msg);
        if (receive_res == 1) {
            continue;
        } else if (receive_res != 0) {
            fprintf(stderr, "Pr %d: Failed to get ACK from transfer, ret code %d\n", pr -> id, receive_res);
            fflush(stderr);
            free_message(&msg);
            return;
        } // sync time because receive event occured
        set_lamport_time(msg);
    }
    while (msg -> s_header.s_type != ACK); // timeout can be set here
    fprintf(stdout, "%d: Process %d: received ACK from process %d\n", get_lamport_time(), get_pr_id(pr), dst);
    fflush(stdout);
    free_message(&msg);
    return;
}

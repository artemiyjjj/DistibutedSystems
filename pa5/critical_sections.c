#include "critical_sections.h"

#include "banking.h"
#include "ipc.h"
#include "ipc_message.h"
#include "lamport_time.h"
#include "pa2345.h"
#include "process_pub.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

/** 
 * В реализации нет поддержки разрешения входа в критическую секцию на основании
 * локальных отметок времени других процессов, полученных от них из сообщений 
 * ЛЮБОГО типа - учёт ведется только на основании сообщений CS_REPLY
 */

bool is_proc_allowed_cs(struct process* self) {
    int min_index = token_arr_get_index_min_timestamp(self -> token_array);
    return min_index == get_pr_id(self) 
        // && token_arr_all_set(self -> token_array)
        && self -> state.reply_msg_received == self -> children_amount - 1;
}


int request_cs(const void * proc) {
    struct process* self = (struct process*) proc;
    Message* msg = NULL;

    if (create_message(CS_REQUEST, 0, NULL, get_lamport_time(), &msg) != 0) {
        return 1;
    }
    if (send_multicast(self, msg) != 0) {
        free_message(&msg);
        return 2;
    }

    token_arr_set(self -> token_array, get_pr_id(self), msg);

    free_message(&msg);
    return 0;
}

int release_cs(const void * proc) {
    struct process* self = (struct process*) proc;
    Message* msg = NULL;

    token_arr_reset_replies(self -> token_array);
    self -> state.reply_msg_received = 0;
    self -> state.is_allowed_cs = false;

    if (create_message(CS_RELEASE, 0, NULL, get_lamport_time(), &msg) != 0) {
        return 1;
    }
    if (send_multicast(self, msg) != 0) {
        return 2;
    }
    token_arr_set(self -> token_array, get_pr_id(self), msg);
    free_message(&msg);
    return 0;
}

int process_reply_cs(struct process* self, const local_id dst) {
    Message* msg = NULL;
    if (create_message(CS_REPLY, 0, NULL, get_lamport_time(), &msg) != 0) {
        return 1;
    }
    if (send(self, dst, msg) != 0) {
        return 2;
    }
    free_message(&msg);
    return 0;
}



void handle_cs_request(struct process* self, Message* msg, local_id from) {
    token_arr_set(self -> token_array, from, msg);

    if (process_reply_cs(self, from) != 0) {
        fprintf(stderr, "%d: proc %d fialed to reply CS REQUEST to %d", get_lamport_time(), get_pr_id(self), from);
    }
}

void handle_cs_reply(struct process* self, Message* msg, local_id from) {
    self -> state.reply_msg_received += 1;
    token_arr_set(self -> token_array, from, msg);
    if (is_proc_allowed_cs(self)) {
        self -> state.is_allowed_cs = true;
    }
}

void handle_cs_release(struct process* self, Message* msg, local_id from) {
    token_arr_set(self -> token_array, from, msg);
    
    if (is_proc_allowed_cs(self)) {
        self -> state.is_allowed_cs = true;
    }
}


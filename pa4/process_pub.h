#ifndef PROCESS_PUB_H
#define PROCESS_PUB_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "lamport_time.h"


struct duplex_chanel_list;

struct process {
    int8_t                      id;
    pid_t                       this_pid;
    pid_t                       parent_pid;
    short                       children_amount;
    struct duplex_chanel_list*  ch_list;
    struct token_arr*           token_array;
    struct state {
        short                   start_msg_received;
        short                   done_msg_received;
        short                   reply_msg_received;
        bool                    is_allowed_cs;
        bool                    is_done_workload;
    } state;
    int                         cs_exec_amount;
};

int8_t get_pr_id(const struct process* const pr);

pid_t get_pr_this_pid(const struct process* const pr);

pid_t get_pr_parent_id(const struct process* const pr);

struct duplex_chanel_list* get_pr_chanel_list(const struct process* const pr);

#endif

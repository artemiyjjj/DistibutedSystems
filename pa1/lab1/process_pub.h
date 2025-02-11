#ifndef PROCESS_PUB_H
#define PROCESS_PUB_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>


struct duplex_chanel_list;

struct process {
    int8_t id;
    pid_t this_pid;
    pid_t parent_pid;
    struct duplex_chanel_list* ch_list;
};

int8_t get_pr_id(const struct process* const pr);

pid_t get_pr_this_pid(const struct process* const pr);

pid_t get_pr_parent_id(const struct process* const pr);

struct duplex_chanel_list* get_pr_chanel_list(const struct process* const pr);

#endif

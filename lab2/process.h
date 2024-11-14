#ifndef PROCESS_H
#define PROCESS_H

#include "banking.h"
#include "chanel.h"
#include "process_pub.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t create_child_process(const int8_t l_id,
                         struct duplex_chanel_list** const ch, balance_t initial_balance);

pid_t create_child_processes(const short processes_amount,
                           struct process* const self, balance_t* initial_balances);

// int wait_for_children(const short processes_amount,
//                       struct process* const self);

int child_process_exec(struct process* const self, const short child_processes_amount);

int parent_process_exec(struct process* const self, const short child_processes_amount);

#endif

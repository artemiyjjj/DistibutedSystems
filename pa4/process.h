#ifndef PROCESS_H
#define PROCESS_H

#include "process_pub.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CS_EXEC_CONST 5


pid_t create_child_processes(const short processes_amount, struct process* const self);

int child_process_exec(struct process* const self, bool use_mutex);

int parent_process_exec(struct process* const self);

int wait_for_children(struct process* const self);

#endif

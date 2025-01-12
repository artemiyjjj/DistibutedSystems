#ifndef BANK_SERVER_H
#define BANK_SERVER_H

#include "ipc.h"
#include "process_pub.h"

extern struct process* lamport_proc;


int handle_transfer(void* self, Message* msg);

void append_balance_history(struct process* self);

timestamp_t get_lamport_time();

void set_lamport_time(Message* msg);

#endif

#ifndef BANK_SERVER_H
#define BANK_SERVER_H

#include "ipc.h"
#include "process_pub.h"


int handle_transfer(struct process* self, Message* msg);

int handle_stop(struct process* self, Message* msg);

void append_pending_balance_history(struct process* self, Message* transfer_message);

void append_balance_history(struct process* self);

timestamp_t get_lamport_time(void);

void set_lamport_time(Message* msg);

#endif

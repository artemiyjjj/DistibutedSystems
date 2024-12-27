#ifndef BANK_SERVER_H
#define BANK_SERVER_H

#include "ipc.h"
#include "process_pub.h"

int handle_transfer(void* self, Message* msg);

void append_balance_history(struct process* self);

#endif

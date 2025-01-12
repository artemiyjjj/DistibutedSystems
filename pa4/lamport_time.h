#ifndef BANK_SERVER_H
#define BANK_SERVER_H

#include "ipc.h"

timestamp_t get_lamport_time(void);

void set_lamport_time(Message* msg);

#endif

#ifndef BANK_SERVER_H
#define BANK_SERVER_H

#include "ipc.h"

#include <stdbool.h>

typedef struct token_lamport {
    timestamp_t     timestamp;
    local_id        proc_id;
} token_lamport;

int token_lamport_cmp(const token_lamport t1, const token_lamport t2);

typedef struct opt_token_lamport {
    token_lamport token;
    bool          is_present;
} opt_token_lamport;



typedef struct token_list {
    token_lamport        token;
    struct token_list*   next;
} token_list;

bool token_list_is_empty(void);

void token_list_append(token_lamport token);

bool token_list_remove(token_lamport token);

bool token_list_remove_by_id(local_id id);

// bool token_list_remove_by_pos(int pos);

// void token_list_remove_min(void);

opt_token_lamport token_list_min(void);

// opt_token_lamport token_list_last(void);


// timestamp_t get_lamport_time(void);

void set_lamport_time(Message* msg);

#endif

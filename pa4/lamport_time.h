#ifndef BANK_SERVER_H
#define BANK_SERVER_H

#include "ipc.h"

#include <stdbool.h>

#define MAX_SHORT 32767

typedef struct token_lamport {
    timestamp_t     timestamp;
    local_id        proc_id;
} token_lamport;

int token_lamport_cmp(const token_lamport t1, const token_lamport t2);

enum token_type {
    T_REQUEST,
    T_REPLY,
    T_UNSET
};

typedef struct token_arr_elem {
    enum token_type     t_type;
    timestamp_t         t_timestamp;
} token_arr_elem;

/**
 * Stores last received tokens from other processes.
 * Index of elements corresponds to their ids. So allocated array 
 * has `child_amount` + 1 elements.
 */
typedef struct token_arr {
    short                 lenght;
    token_arr_elem      elements[];
} token_arr;

token_arr* token_arr_init(short arr_lenght);

void token_arr_destroy(token_arr* arr);

token_arr_elem* token_arr_get(token_arr* arr, int index);

int token_arr_get_index_min_timestamp(token_arr* arr);

void token_arr_set(token_arr* arr, int index, Message* msg);

void token_arr_reset_replies(token_arr* arr);

void token_arr_log_arr(token_arr* arr);

// bool token_arr_are_all_set(token_arr* arr);


// typedef struct token_list {
//     token_lamport        token;
//     struct token_list*   next;
// } token_list;

// typedef struct opt_token_lamport {
//     token_lamport token;
//     bool          is_present;
// } opt_token_lamport;

// bool token_list_is_empty(void);

// void token_list_append(token_lamport token);

// bool token_list_remove(token_lamport token);

// bool token_list_remove_by_id(local_id id);

// bool token_list_remove_by_pos(int pos);

// void token_list_remove_min(void);

// opt_token_lamport token_list_min(void);

// opt_token_lamport token_list_last(void);


// timestamp_t get_lamport_time(void);

void set_lamport_time(Message* msg);

#endif

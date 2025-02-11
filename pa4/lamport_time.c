#include "lamport_time.h"

#include "banking.h"
#include "ipc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SHORT 32767

// Local to each process state of logical time
static timestamp_t local_lamport_time = 0;

// static token_list* local_token_list = NULL;


int token_lamport_cmp(const token_lamport t1, const token_lamport t2) {
    if (t1.timestamp > t2.timestamp) {
        return 1;
    } else if (t1.timestamp < t2.timestamp) {
        return -1;
    } else if (t1.proc_id > t2.proc_id) {
        return 1;
    } else if (t1.proc_id < t2.proc_id) {
        return -1;
    } else {
        return 0;
    }
}

timestamp_t get_lamport_time(void) {
    return local_lamport_time;
}

/**
 * @brief Sync clock if msg provided and increment the lamport clock
 * 
 * @param self 
 * @param received_msg struct Message entity for receiveing event, NULL if inner or sending event
 */
void set_lamport_time(Message *received_msg) {
    timestamp_t cur_time = get_lamport_time();
    timestamp_t recieved_time = 0;

    local_lamport_time += 1;
    if (received_msg == NULL) {
        return;
    } else {
        recieved_time = received_msg -> s_header.s_local_time;
    }

    if (recieved_time >= cur_time) {
        local_lamport_time = recieved_time;
        local_lamport_time += 1;
    }
}

token_arr* token_arr_init(short arr_lenght) {
    token_arr* arr = malloc(sizeof(token_arr) + ((arr_lenght + 1) * sizeof(token_arr_elem)));
    arr -> lenght = arr_lenght;
    return arr;
}

void token_arr_destroy(token_arr* arr) {
    free(arr);
}

token_arr_elem* token_arr_get(token_arr* arr, int index) {
    return index <= arr -> lenght ? &(arr -> elements[index]) : NULL; 
}

int token_arr_get_index_min_timestamp(token_arr* arr) {
    timestamp_t cur_timestamp;
    enum token_type cur_type;
    timestamp_t min_timestamp = MAX_SHORT;
    int min_index = -1;

    for (int i = 1; i <= arr -> lenght; i++) {
        cur_timestamp = arr -> elements[i].t_timestamp;
        cur_type = arr -> elements[i].t_type;
        // Encount only request since replies are always bigger than cur timestamp
        if (cur_type == T_REQUEST && cur_timestamp < min_timestamp) {
            min_timestamp = cur_timestamp;
            min_index = i;
        }
    }
    return min_index;
}

/**
 * @brief Update internal proc's queue with newly received message
 *
 * Assign changes only if certain consistency conditions satisfied.
 * These conditions are based on lamport algorithm specifics.
 * 
 * @param arr 
 * @param index 
 * @param msg 
 */
void token_arr_set(token_arr* arr, int index, Message* msg) {
    if (index > arr->lenght) {
        fprintf(stderr, "INTERNAL ERROR: pr%d: out of bounds in arr token\n", index);
        return;
    } 
    token_arr_elem* arr_elem = &(arr -> elements[index]);
    MessageType msg_type = msg->s_header.s_type;
    // Request can be received only after release or at start
    if (msg_type == CS_REQUEST) {
        arr_elem -> t_timestamp = msg -> s_header.s_local_time;
        arr_elem -> t_type = T_REQUEST;
    // Reply can be received after request or after last release and shouldn't override request
    } else if (msg_type == CS_REPLY) {
        if (arr_elem -> t_type == T_REQUEST) {
            return;
        } 
        arr_elem -> t_timestamp = msg -> s_header.s_local_time;
        arr_elem -> t_type = T_REPLY;
    // Release can be received after request and reply so reply would last
    } else if (msg_type == CS_RELEASE) {
        arr_elem -> t_type = T_UNSET;
    } else {
        fprintf(stderr, "INTERNAL WARNING: inconsistent token state\n");
        return;
    }
}

/**
 * @brief Leave Requests and Unset values in queue 
 * 
 * @param arr 
 */
void token_arr_reset_replies(token_arr* arr) {
    token_arr_elem* arr_elem;
    for (int i = 1; i <= arr -> lenght; i++) {
        arr_elem = &(arr -> elements[i]);
        if (arr_elem -> t_type == T_REPLY) {
            arr_elem -> t_type = T_UNSET;
        }
    }
}

// bool token_arr_are_all_set(token_arr* arr) {
//     for (int i = 1; i <= arr->lenght; i++) {
//         if (arr->elements[i].t_type == T_UNSET) {
//             return false;
//         }
//     }
//     return true;
// }


// bool token_list_is_empty(void) {
//     return local_token_list == NULL;
// }

// void token_list_append(token_lamport token) {
//     token_list* cur_t_l = local_token_list;

//     if (cur_t_l == NULL) {
//         local_token_list = malloc(sizeof(token_list));
//         local_token_list->token = token;
//         local_token_list->next = NULL;
//         return;
//     }
//     while (cur_t_l -> next != NULL) { // find last
//         cur_t_l = cur_t_l -> next;
//     }
//     cur_t_l -> next = malloc(sizeof(token_list));
//     cur_t_l -> next -> token = token;
//     cur_t_l -> next -> next = NULL;
// }

// bool token_list_remove(token_lamport token) {
//     token_list* cur_t_l = local_token_list;
//     token_list* prev_t_l = NULL;
    

//     if (cur_t_l == NULL) {
//         return false;
//     }
//     while (cur_t_l != NULL) { // find last
//         if (token_lamport_cmp(cur_t_l -> token, token) != 0) {
//             prev_t_l = cur_t_l;
//             cur_t_l = cur_t_l -> next;
//             continue;
//         }
//         if (prev_t_l != NULL) {
//             prev_t_l -> next = cur_t_l -> next;
//         } else {
//             local_token_list = cur_t_l -> next;
//         }
//         free(cur_t_l);
//         return true;
//     }
//     return false;
// }

// bool token_list_remove_by_id(local_id id) {
//     token_list* cur_t_l = local_token_list;
//     token_list* prev_t_l = NULL;
//     if (token_list_is_empty()) {
//         return false;
//     }

//     while (cur_t_l != NULL) {
//         if (cur_t_l -> token.proc_id == id) {
//             if (prev_t_l == NULL) {
//                 local_token_list = cur_t_l -> next;
//             } else {
//                 prev_t_l -> next = cur_t_l -> next;
//             }
//             free(cur_t_l);
//             return true;
//         }
//     }
//     return false;
// }

// bool token_list_remove_by_pos(int pos) {
//     token_list* cur_l_t = local_token_list;
//     token_list* prev_t_l = NULL;
//     int pos_counter = 0;
//     if (cur_l_t == NULL) {
//         return false;
//     }
//     while (cur_l_t != NULL) {
//         if (pos_counter == pos) {
//             if (prev_t_l == NULL) {
//                 local_token_list = cur_l_t -> next;
//             } else {
//                 prev_t_l -> next = cur_l_t -> next;
//             }
//             free(cur_l_t);
//             return true;
//         }
//         prev_t_l = cur_l_t;
//         cur_l_t = cur_l_t -> next;
//         pos_counter += 1;
//     }
//     return false;
// }

// void token_list_remove_min(void) {
//     token_list* cur_t_l = local_token_list;
//     token_lamport min_token = {.timestamp = MAX_SHORT, .proc_id = MAX_PROCESS_ID};
//     int min_position = 0;
//     int cur_position = 0;

//     while (cur_t_l != NULL) {
//         if (token_lamport_cmp(cur_t_l -> token, min_token) < 0) {
//             min_token = cur_t_l -> token;
//             min_position = cur_position;
//         }
//         cur_t_l = cur_t_l -> next;
//         cur_position++;
//     }
//     token_list_remove_by_pos(min_position);
// }

// /**
//  * @brief Do not use with empty list
//  * 
//  * @return token_lamport 
//  */
// opt_token_lamport token_list_min(void) {
//     token_list* cur_t_l = local_token_list;
//     token_lamport min_token = {.timestamp = MAX_SHORT, .proc_id = MAX_PROCESS_ID};
//     opt_token_lamport ret_val = {.token = min_token, .is_present = false};

//     while (cur_t_l != NULL) {
//         if (token_lamport_cmp(cur_t_l -> token, ret_val.token) < 0) {
//             ret_val.token = cur_t_l -> token;
//             ret_val.is_present = true;
//         }
//         cur_t_l = cur_t_l -> next;
//     }
//     return ret_val;
// }

// opt_token_lamport token_list_last(void) {
//     token_list* cur_t_l = local_token_list;
//     opt_token_lamport opt_token = {.is_present = false};

//     if (cur_t_l == NULL) {
//         return opt_token;
//     }

//     while (cur_t_l -> next != NULL) {
//         cur_t_l = cur_t_l -> next;
//     }
//     opt_token.is_present = true;
//     opt_token.token = cur_t_l->token;
//     return opt_token;
// }

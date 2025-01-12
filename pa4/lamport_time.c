#include "banking.h"
#include "ipc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local to each process state of logical time
static timestamp_t local_lamport_time = 0;


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

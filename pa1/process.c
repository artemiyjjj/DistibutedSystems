#include "process.h"

#include "chanel.h"
#include "ipc.h"
#include "ipc_message.h"
#include "pa1.h"
#include "process_pub.h"

#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int8_t get_pr_id(const struct process* const pr) {
    return pr -> id;
}

pid_t get_pr_this_pid(const struct process* const pr) {
    return pr -> this_pid;
}


pid_t get_pr_parent_id(const struct process* const pr) {
    return pr -> parent_pid;
}


struct duplex_chanel_list* get_pr_chanel_list(const struct process* const pr) {
    return pr -> ch_list;
}


static int close_irrelevant_chanels(struct process* self) {                             
    struct duplex_chanel_list* cur_d_ch_list = self -> ch_list;
    struct duplex_chanel_list* prev_ch_list = NULL;
    struct duplex_chanel_list* next_ch_list = NULL;
    int8_t l_id = self -> id;

    while (cur_d_ch_list != NULL) {
        if (cur_d_ch_list -> first_id != l_id && cur_d_ch_list -> second_id != l_id) {
            next_ch_list = cur_d_ch_list -> next;
            if (close_duplex_chanel(cur_d_ch_list) != 0) {
                return -1;
            }
            if (prev_ch_list != NULL) {
                free(prev_ch_list -> next);
                prev_ch_list -> next = next_ch_list;
            } else {
                free(self -> ch_list);
                self -> ch_list = next_ch_list;
            }
            cur_d_ch_list = next_ch_list;
            continue;
        }
        else if (cur_d_ch_list -> first_id == l_id) {
            if (close_chanel_part(cur_d_ch_list, 2) != 0) {
                return -1;
            }
        }
        else if (cur_d_ch_list -> second_id == l_id) {
            if (close_chanel_part(cur_d_ch_list, 1) != 0) {
                return -1;
            }
        }
        prev_ch_list = cur_d_ch_list;
        cur_d_ch_list = cur_d_ch_list -> next;
    }
    return 0;
}

/**
 * @brief Create a child processes
 * 
 * @param processes_amount amount of child processes to create
 * @param ch_list NULLABLE pointer to pointer to list of duplex chanels
 * @return `>= 0` - pid of returned process, `-1` - bad process amount,
 * @return  `-2` failed to open pipes, `-3` - failed to close unused pipes.
 */
pid_t create_child_processes(const short processes_amount,
                           struct process* const self) {
    if (processes_amount > MAX_PROCESS_ID || processes_amount < 0) {
        return -1;
    }

    // int             created_pr_amount = 0;
    // local_id        created_processes[processes_amount + 1];
    local_id        cur_id = PARENT_ID + 1;
    const local_id  max_id = PARENT_ID + processes_amount;

    // created_processes[0] = PARENT_ID;
    
    while (cur_id <= max_id) {
        self -> this_pid = fork();
        if (self -> this_pid == 0) { // child
            self -> id = cur_id;
            if (close_irrelevant_chanels(self) != 0) {
                return -3;
            }
            return self -> this_pid;   // Child's path
        } else if (self -> this_pid < 0) {
            // other processes get killed when parent returns from main()
            return -2; // Failed to create
        } // parent
        // created_pr_amount++;
        // created_processes[created_pr_amount] = cur_id;
        cur_id++;
    }
    self -> id = PARENT_ID;
    if (close_irrelevant_chanels(self) != 0) {
        return -3;
    }
    return self -> this_pid;
}


/**
 * @brief Workload of a child process
 * 
 * @param self child process information
 * @param child_processes_amount amount of child processes in system including current 
 * @return int 0 - success
 * @return int 1 - failed to allocate memory
 * @return int 2 - failed to send multicast msg
 * @return int 3 - failed to receive a msg
 * @return int 4 - received msg of unexected type  
 */
int child_process_exec(struct process* const self, const short child_processes_amount) {
    Message* msg;
    char* msg_contents;
    short start_msg_received = 0;
    short done_msg_received = 0;
    const short receive_pr_wait_amount = child_processes_amount - 1;
    int receive_any_res;

    // log started
    msg_contents = malloc(strlen(log_started_fmt));
    if (msg_contents == NULL) {
        return 1;
    }
    sprintf(msg_contents, log_started_fmt, self ->id, self -> this_pid, self -> parent_pid);
    fprintf(stdout, "%s", msg_contents);
    // craft message
    if (create_mesage(STARTED, strlen(msg_contents), msg_contents, &msg) != 0) {
        free(msg_contents);
        return 1;
    }
    if (send_multicast(self, msg) != 0) {
        fprintf(stderr, "Failed to send multicast message from process %d\n", self -> id);
        free(msg_contents);
        free_message(&msg);
        return 2;
    }
    free(msg_contents);
    free_message(&msg);
    if (create_empty_msg(&msg) != 0) {
        return 1;
    }
    
    // fprintf(stdout, "Pr %d waits for %d msgs\n", get_pr_id(self), receive_pr_wait_amount);
    while (start_msg_received < receive_pr_wait_amount) {
        receive_any_res = receive_any(self, msg);
        if (receive_any_res == 2) { // no msg is sent
            sleep(1);
            continue;
        }
        else if (receive_any_res != 0) {
            fprintf(stderr, "Failed to receive any msg in process %d, code %d, \n", self -> id, receive_any_res);
            free_message(&msg);
            return 3;
        }
        // Validate msg
        if (msg -> s_header.s_type == STARTED) {
            start_msg_received++;
        } else if (msg -> s_header.s_type == DONE) {
            done_msg_received++;
        } else {
            fprintf(stderr, "Process %d received msg of unexpected type", get_pr_id(self));
            free_message(&msg);
            return 4;
        }
        memset(msg, 0, MAX_MESSAGE_LEN);
    }
    free_message(&msg);

    // log all started received
    fprintf(stdout, log_received_all_started_fmt, self -> id);

    // craft and log done msg
    msg_contents = malloc(strlen(log_done_fmt));
    if (msg_contents == NULL) {
        return 1;
    }
    sprintf(msg_contents, log_done_fmt, self -> id);
    fprintf(stdout, "%s", msg_contents);

    if (create_mesage(DONE, strlen(msg_contents), msg_contents, &msg) != 0) {
        free(msg_contents);
        return 1;
    }
    // send done msg
    if (send_multicast(self, msg) != 0) {
        free(msg_contents);
        free_message(&msg);
        return 2;
    }
    free(msg_contents);
    free_message(&msg);

    // create empty msg for receiving
    if (create_empty_msg(&msg) != 0) {
        return 1;
    }
    while (done_msg_received < receive_pr_wait_amount) {
        receive_any_res = receive_any(self, msg);
        if (receive_any_res == 2) { // no msg is sent
            sleep(1);
            continue;
        }
        else if (receive_any_res != 0) {
            free_message(&msg);
            return 3;
        }
        // Validate msg
        if (msg -> s_header.s_type == DONE) {
            done_msg_received++;
        } else {
            free_message(&msg);
            return 4;
        }
        memset(msg, 0, MAX_MESSAGE_LEN);
    }
    free_message(&msg);
    fprintf(stdout, log_received_all_done_fmt, self -> id);

    return 0;
}

/**
 * @brief Block until all child processes exit
 * 
 * @param processes_amount value to count processes
 * @param self pointer on the process structure
 * @return 0 on success, any non-zero value on error 
 */
static int wait_for_children(const short processes_amount,
                      struct process* const self) {
    int freed_pr_amount = 0;
    
    while (freed_pr_amount != processes_amount) {
        wait(NULL);
        freed_pr_amount++;
    }
    return 0;
}

/**
 * @brief Workload of a parent process
 * 
 * @param self parent process information
 * @param child_processes_amount amount of child processes in system to wait to
 * @return int 0 - success
 * @return int 1 - failed to allocate memory
 * @return int 2 - failed to receive a msg
 * @return int 3 - received msg of unexected type  
 */
int parent_process_exec(struct process* const self, const short child_processes_amount) {
    Message* msg;
    short start_msg_received = 0;
    short done_msg_received = 0;
    int receive_any_res;

    // create empty msg for receiving
    if (create_empty_msg(&msg) != 0) {
        return 1;
    }

    // fprintf(stdout, "Pr %d waits for %d msgs\n", get_pr_id(self), child_processes_amount);
    while (start_msg_received < child_processes_amount) {
        receive_any_res = receive_any(self, msg);
        if (receive_any_res == 2) { // no msg is sent
            sleep(1);
            continue;
        } else if (receive_any_res != 0) {
            fprintf(stderr, "Failed to receive any msg in process %d, code %d, \n", self -> id, receive_any_res);
            free_message(&msg);
            return 2;
        }
        // Validate msg
        if (msg -> s_header.s_type == STARTED) {
            start_msg_received++;
        } else if (msg -> s_header.s_type == DONE) {
            done_msg_received++;
        } else {
            fprintf(stderr, "Process %d received msg of unexpected type", get_pr_id(self));
            free_message(&msg);
            return 3;
        }
        memset(msg, 0, MAX_MESSAGE_LEN);
    }
    while (done_msg_received < child_processes_amount) {
        receive_any_res = receive_any(self, msg);
        if (receive_any_res == 2) { // no msg is sent
            sleep(1);
            continue;
        } else if (receive_any_res != 0) {
            free_message(&msg);
            return 2;
        }
        // Validate msg
        if (msg -> s_header.s_type == DONE) {
            done_msg_received++;
        } else {
            free_message(&msg);
            return 3;
        }
        memset(msg, 0, MAX_MESSAGE_LEN);
    }
    free_message(&msg);
    return wait_for_children(child_processes_amount, self);
}

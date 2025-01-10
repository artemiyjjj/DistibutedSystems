#include "process.h"

#include "bank_server.h"
#include "banking.h"
#include "chanel.h"
#include "ipc.h"
#include "ipc_message.h"
#include "pa2345.h"
#include "process_pub.h"

#include <assert.h>
#include <stdint.h>
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
    int8_t l_id = get_pr_id(self);

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
 * @return int 1 - failed to allocate memory
 */
pid_t create_child_processes(const short processes_amount, struct process* const self, balance_t* initial_balances) {
    if (processes_amount > MAX_PROCESS_ID || processes_amount < 0) {
        return -1;
    }

    local_id        cur_id = PARENT_ID + 1;
    const local_id  max_id = PARENT_ID + processes_amount;
    pid_t ret_pid;

    // created_processes[0] = PARENT_ID;
    
    while (cur_id <= max_id) {
        ret_pid = fork();
        if (ret_pid == 0) { // child
            self -> this_pid = getpid();
            self -> parent_pid = getppid();
            self -> id = cur_id;
            assert (close_irrelevant_chanels(self) == 0);
            self -> local_balance = initial_balances[cur_id - 1];
            self -> balanceHistory = malloc(sizeof(BalanceHistory));
            assert(self -> balanceHistory != NULL);
            self -> balanceHistory -> s_history_len = 0;
            self -> balanceHistory -> s_id = cur_id;
            append_balance_history(self);
            return ret_pid;   // Child's path
        } else if (ret_pid < 0) {
            // other processes get killed when parent returns from main()
            return -2; // Failed to create
        } // parent
        cur_id++;
    }
    self -> this_pid = getpid();
    self -> parent_pid = getppid();
    self -> id = PARENT_ID;
    assert (close_irrelevant_chanels(self) == 0);
    return ret_pid;
}


/**
 * @brief Workload of a child process
 * 
 * @param self child process information
 * @param child_processes_amount amount of child processes in system including current 
 * @return int 0 - success
 * @return int 1 - failed to allocate memory
 * @return int 2 - failed to send msg
 * @return int 3 - failed to receive a msg
 * @return int 4 - received msg of unexected type
 * @return int 5 - failed to handle transfer procedure
 */
int child_process_exec(struct process* const self, const short child_processes_amount) {
    Message* msg;
    Message* send_msg;
    char* msg_contents;
    short start_msg_received = 0;
    short done_msg_received = 0;
    const short receive_pr_wait_amount = child_processes_amount - 1;
    // BalanceState b_state = {.s_balance = self -> local_balance, .s_balance_pending_in = 0, .s_time = 0};
    int receive_any_res;
    // int debug = 1;

    // log started
    msg_contents = malloc(strlen(log_started_fmt)*2);
    if (msg_contents == NULL) {
        return 1;
    }
    // printf( "%d: pr: bal: %d")
    sprintf(msg_contents, log_started_fmt, get_lamport_time(), get_pr_id(self), self -> this_pid, self -> parent_pid, self -> local_balance);
    fprintf(stdout, "%s", msg_contents);
    fprintf(log_events_stream, "%s", msg_contents);
    // craft message
    set_lamport_time(NULL);
    if (create_message(STARTED, strlen(msg_contents), msg_contents, get_lamport_time(), &msg) != 0) {
        free(msg_contents);
        return 1;
    }
    if (send_multicast(self, msg) != 0) {
        fprintf(stderr, "Failed to send multicast message from process %d\n", get_pr_id(self));
        return 2;
    }
    free(msg_contents);
    free_message(&msg);
    
    // Receive all STARTED messages
    if (create_empty_msg(&msg) != 0) {
        return 1;
    }
    while (start_msg_received < receive_pr_wait_amount) {
        receive_any_res = receive_any(self, msg);
        if (receive_any_res == 2) { // no msg is sent
            continue;
        }
        else if (receive_any_res != 0) {
            fprintf(stderr, "Failed to receive any msg in process %d, ret code %d, \n", get_pr_id(self), receive_any_res);
            free_message(&msg);
            return 3;
        }
        
        set_lamport_time(msg);
        // Validate msg
        if (msg -> s_header.s_type == STARTED) {
            start_msg_received++;
        }
        else if (msg -> s_header.s_type == TRANSFER) {
            if (handle_transfer(self, msg) != 0) {
                fprintf(stderr, "Pr %d: failed to handle transfer\n", get_pr_id(self));
                free_message(&msg);
                return 5;
            }
        } else {
            fprintf(stderr, "Process %d received msg of unexpected type %d\n", get_pr_id(self), msg -> s_header.s_type);
            free_message(&msg);
            return 4;
        }
        memset(msg, 0, MAX_MESSAGE_LEN);
    }
    free_message(&msg);

    // log all started received
    fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), get_pr_id(self));
    fprintf(log_events_stream, log_received_all_started_fmt, get_lamport_time(), get_pr_id(self));

    // Prepare for bank workload
    if (create_empty_msg(&msg) != 0) {
        return 1;
    }
    while (done_msg_received < receive_pr_wait_amount) {
        receive_any_res = receive_any(self, msg);
        if (receive_any_res == 2) { // no msg is sent
            continue;
        }
        else if (receive_any_res != 0) {
            fprintf(stderr, "Process %d: failed to receive transfer\n", get_pr_id(self));
            free_message(&msg);
            return 3;
        }

        // for receive
        set_lamport_time(msg);
        // Validate msg
        if (msg -> s_header.s_type == DONE) {
            done_msg_received++;
        }
        else if (msg -> s_header.s_type == TRANSFER) {
            if (handle_transfer(self, msg) != 0) {
                fprintf(stderr, "Pr %d: failed to handle transfer\n", get_pr_id(self));
                return 5;
            }
        }
        // craft and log done msg, end of balanceHistory
        else if (msg -> s_header.s_type == STOP) {
            if (handle_stop(self, msg) != 0) {
                fprintf(stderr, "Pr %d: Failed to handle stop\n", get_pr_id(self));
                return 5;
            }
        }
        else { // Unexpected msg type
            fprintf(stderr, "Process %d received msg of unexpected type %d\n", get_pr_id(self), msg -> s_header.s_type);
            free_message(&msg);
            return 4;
        }
        memset(msg, 0, MAX_MESSAGE_LEN);
    }
    
    free_message(&msg);
    fprintf(stdout, log_received_all_done_fmt, get_lamport_time(), get_pr_id(self));
    fprintf(log_events_stream, log_received_all_done_fmt, get_lamport_time(), get_pr_id(self));

    // aggregate and send BALANCE_HISTORY after all DONEs received
    set_lamport_time(NULL);
    ssize_t balance_history_size = sizeof(BalanceHistory) - (sizeof(BalanceState) * (MAX_T + 1 - self -> balanceHistory -> s_history_len));
    if (create_message(BALANCE_HISTORY, balance_history_size, self -> balanceHistory, get_lamport_time(), &send_msg) != 0) {
        return 1;
    }
    if (send(self, PARENT_ID, send_msg) != 0) {
        return 2;
    }
    free_message(&send_msg);

    free(self -> balanceHistory);
    return 0;
}

/**
 * @brief Block until all child processes exit
 * 
 * @param processes_amount value to count processes
 * @param self pointer on the process structure
 * @return 0 on success, any non-zero value on error 
 */
int wait_for_children(const short processes_amount, struct process* const self) {
    int child_index = 0;
    int cur_status;

    while (child_index != processes_amount) {
        waitpid(-1, &cur_status, 0);
        child_index++;
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
 * @return int 4 - Failed to send STOP msg
 */
int parent_process_exec(struct process* const self, const short child_processes_amount) {
    Message* msg = NULL;
    AllHistory* all_history = NULL;
    short start_msg_received = 0;
    short hist_msg_received = 0;
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
            continue;
        } else if (receive_any_res != 0) {
            fprintf(stderr, "Failed to receive any msg in process %d, code %d, \n", get_pr_id(self), receive_any_res);
            free_message(&msg);
            return 2;
        }
        // Validate msg
        set_lamport_time(msg);
        if (msg -> s_header.s_type == STARTED) {
            start_msg_received++;
        } else {
            fprintf(stderr, "Process %d received msg of unexpected type", get_pr_id(self));
            free_message(&msg);
            return 3;
        }
        memset(msg, 0, MAX_MESSAGE_LEN);
    }
    free_message(&msg);

    // log all started received
    fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), get_pr_id(self));
    fprintf(log_events_stream, log_received_all_started_fmt, get_lamport_time(), get_pr_id(self));
    

    // bank client workload
    bank_robbery(self, PARENT_ID + child_processes_amount);

    // send STOP msg
    set_lamport_time(NULL);
    create_message(STOP, 0, NULL, get_lamport_time(), &msg);
    if (msg == NULL) {
        return 1;
    }
    if (send_multicast(self, msg) != 0) {
        return 4;
    }
    free_message(&msg);

    // wait for DONE messages
    if (create_empty_msg(&msg) != 0) {
        return 1;
    }
    BalanceHistory* cur_balance_history_ptr = NULL;
    all_history = malloc(sizeof(AllHistory));
    if (all_history == NULL) {
        return 1;
    }
    all_history -> s_history_len = 0;
    while (done_msg_received < child_processes_amount || hist_msg_received < child_processes_amount) {
        receive_any_res = receive_any(self, msg);
        if (receive_any_res == 2) { // no msg is sent
            continue;
        } else if (receive_any_res != 0) {
            free_message(&msg);
            return 2;
        }
        // Validate msg
        set_lamport_time(msg);
        if (msg -> s_header.s_type == DONE) {
            done_msg_received++;
        } else if (msg -> s_header.s_type == BALANCE_HISTORY) {
            cur_balance_history_ptr = &all_history -> s_history[all_history -> s_history_len];
            memcpy(cur_balance_history_ptr, msg -> s_payload, msg -> s_header.s_payload_len);
            all_history -> s_history_len++;
            memset(msg, 0, MAX_MESSAGE_LEN);
            hist_msg_received++;
        } else {
            free_message(&msg);
            return 3;
        }
        memset(msg, 0, MAX_MESSAGE_LEN);
    }
    free_message(&msg);

    print_history(all_history);
    free(all_history);

    return wait_for_children(child_processes_amount, self);
}

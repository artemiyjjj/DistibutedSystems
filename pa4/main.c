#include "chanel.h"
#include "common.h"
#include "ipc.h"
#include "process.h"
#include "process_pub.h"

#include <getopt.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

FILE*      log_events_stream = NULL;
FILE*      log_pipes_stream = NULL;

const char* const SHORT_OPTS = "mp:";

const struct option LONG_OPTS[] = {
    {"processes", required_argument, NULL, 'p'},
    {"mutexl", no_argument, NULL, 'm'},
    {NULL, 0, NULL, 0}
};

/**
 * @brief Example of library usage for lab1
 * 
 * @param argc 
 * @param argv 
 * @return int -1 - Failed to open log file streams
 * @return int 0 - Success
 * @return int 1 - Provided unknown parameter
 * @return int 2 - Provided invalid amount of child processes to create
 * @return int 3 - Failed to create chanels for processes
 * @return int 4 - Proc execution is failed
 * @return int 5 - Failed to create child process(es)
 * @return int 6 - Failed to allocate memory
 */
int main(int argc, char** argv) {
    int                         opt_res;
    extern char*                optarg;
    extern int                  optopt;
    extern int                  optind;
    int                         ret_val = 0;
    short                       child_process_amount = -1;
    bool                        use_mutex = false;
    struct process              current_process = {0};
    int                         proc_work_ret;

    while (opt_res = getopt_long(argc, argv, SHORT_OPTS, LONG_OPTS, &optind), opt_res != -1) {
        switch (opt_res) {
            case 'p': {
                child_process_amount = atoi(optarg);
            } break;
            case 'm': {
                use_mutex = true;
            } break;
            case '?':
            default: {
                return 1;
            }
        }
    }

    if (child_process_amount == -1) {
        fprintf(stderr, "Child processes amount is not set\n");
        return 2;
    } else if (child_process_amount > MAX_PROCESS_ID) {
        fprintf(stderr, "Child process amount should be less than %d, provided: %d\n", MAX_PROCESS_ID, child_process_amount);
        return 2;
    }

    /// Todo change
    log_events_stream = fopen(events_log, "w"); // freopen(events_log, "w", stdout);
    log_pipes_stream = fopen(pipes_log, "w");
    if (log_pipes_stream == NULL || log_events_stream == NULL) {
        fprintf(stderr, "Failed to open log files.\n");
        return -1;
    }
    
    // Create all pipes before creating processes
    int open_chanels_res = open_chanels_for_processes(child_process_amount + 1, PARENT_ID,
                                     PARENT_ID + child_process_amount, &(current_process.ch_list));
    if (open_chanels_res != 0) {
        return 3;
    }
    log_all_pipes_desc(current_process.ch_list, log_pipes_stream);

    // Fork child processes
    pid_t ret_pid = create_child_processes(child_process_amount, &current_process);
    if (ret_pid == 0) { // Child path
        proc_work_ret = child_process_exec(&current_process, use_mutex);
        if (proc_work_ret != 0) {
            fprintf(stderr, "Process %d failed child exec with code %d\n", get_pr_id(&current_process), proc_work_ret);
            ret_val = 4;
        }
        sleep(2); // chance for receiving process to get last message
    } else if (ret_pid > 0) { // parent path
        // receive all child messages and wait for child process to exit
        proc_work_ret = parent_process_exec(&current_process);
        if (proc_work_ret != 0) {
            fprintf(stderr, "Process %d: Failed to execute parent workload, ret code %d\n", get_pr_id(&current_process), proc_work_ret);
            ret_val = 4;
        }
    } else {
        fprintf(stderr, "Failed to create child processes\n");
        ret_val = 5;
    }

    process_destroy(&current_process);
    fclose(log_events_stream);
    fclose(log_pipes_stream);

    return ret_val;
}


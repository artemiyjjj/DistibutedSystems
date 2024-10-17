#include "chanel.h"
#include "common.h"
#include "ipc.h"
#include "process.h"

#include <getopt.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


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
 * @return int 4 - Child execution is failed
 * @return int 5 - Failed to create child process(es)
 */
int main(int argc, char** argv) {
    int                         opt_res;
    extern char*                       optarg;
    int                         ret_val = 0;
    short                       child_process_amount = -1;
    struct process              current_process = {
        .this_pid = getpid(),
        .parent_pid = getppid(),
        .ch_list = NULL
    };
    FILE*      log_events_stream;
    FILE*      log_pipes_stream;
    

    while (opt_res = getopt(argc, argv, "p:"), opt_res != -1) {
        switch (opt_res) {
            case 'p': {
                child_process_amount = atoi(optarg);
                break;
            };
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

    log_events_stream = freopen(events_log, "w", stdout);
    log_pipes_stream = fopen(pipes_log, "w");
    if (log_pipes_stream == NULL|| log_events_stream == NULL) {
        return -1;
    }
    
    // Create all pipes before createing processes
    int open_chanels_res = open_chanels_for_processes(child_process_amount + 1, PARENT_ID,
                                     PARENT_ID + child_process_amount, &(current_process.ch_list));
    if (open_chanels_res != 0) {
        // 
        return 3;
    }
    log_all_pipes_desc(current_process.ch_list, log_pipes_stream);

    // Fork child processes
    pid_t ret_pid = create_child_processes(child_process_amount, &current_process);
    if (ret_pid == 0) { // Child path
        current_process.parent_pid = current_process.this_pid;
        current_process.this_pid = ret_pid;
        if (child_process_exec(&current_process, child_process_amount) != 0) {
            ret_val = 4;
        }
        sleep(1);
    } else if (ret_pid > 0) { // parent path
        // receive all child messages and wait for child process to exit
        if (parent_process_exec(&current_process, child_process_amount) != 0) {
            ret_val = 3;
        }
    } else {
        fprintf(stderr, "Failed to create child processes\n");
        ret_val = 5;
    }
   
    close_all_duplex_chanels(&(current_process.ch_list));
    fclose(log_events_stream);
    fclose(log_pipes_stream);

    return ret_val;
}


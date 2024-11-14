#include "ipc.h"

#include "chanel.h"
#include "process_pub.h"

#include <errno.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char* const logmsg_received_msg = "Process %d: received msg of type %d and length %lu from process %d\n";

const char* const logmsg_send_msg = "Process %d: send msg of type %d and length %lu to process %d\n";

const char* const errmsg_empty_process = "Failed: attempt to send msg using invalid process\n";

const char* const errmsg_ch_not_found = "Failed: chanel between %d and %d is not found\n";


static void log_msg(const Message* const msg, const bool is_received, const local_id receiver, const local_id sender) {
    if (is_received) {
        fprintf(stdout, logmsg_received_msg, receiver, msg -> s_header.s_type, msg -> s_header.s_payload_len + sizeof (MessageHeader), sender);
    } else {
        fprintf(stdout, logmsg_send_msg, sender, msg -> s_header.s_type, msg -> s_header.s_payload_len + sizeof (MessageHeader), receiver);
    }
    fflush(stdout);
}

int send(void * self, local_id dst, const Message * msg) {
    struct process* pr = (struct process*) self;
    struct duplex_chanel_list* cur_list;
    int receiver_fd = -1;
    int write_res;

    if (pr == NULL) {
        fprintf(stderr, errmsg_empty_process);
        return 1;
    }

    if (msg == NULL) {
        fprintf(stderr, "Failed: tried to send NULL msg from %d to %d.\n", get_pr_id(pr), dst);
        return 1;
    } else if (msg -> s_header.s_magic != MESSAGE_MAGIC) {
        fprintf(stderr, "Failed: tried to send invalid msg from %d to %d.\n", get_pr_id(pr), dst);
        return 2;
    }

    cur_list = get_pr_chanel_list(pr);
    while (cur_list != NULL) {
        if (cur_list -> first_id == dst) {
            receiver_fd = cur_list -> d_ch -> ch1[1];
        } else if (cur_list -> second_id == dst) {
            receiver_fd = cur_list -> d_ch -> ch2[1];
        } else {
            cur_list = cur_list -> next;
            continue;
        }
        break;
    }
    if (receiver_fd == -1) {
        fprintf(stderr, "Failed to find chanel from %d to %d.\n", pr -> id, dst);
        return 4;
    }
    // If receiver has been found 
    log_msg(msg, false, dst, pr -> id);
    size_t msg_length = sizeof(MessageHeader) + msg -> s_header.s_payload_len;
    size_t bytes_written = 0;
    while (bytes_written < msg_length) {
        write_res = write(receiver_fd, msg, msg_length);
        // printf("Wr res %d\n", write_res);
        if (write_res == -1) {
            fprintf(stderr, "Failed to send msg from %d to %d using chanel type %d, fd %d: %s.\n", pr -> id, dst, cur_list -> state, receiver_fd, strerror(errno));
            return 3;
        }
        bytes_written += write_res;
    }

    // fprintf(stdout, "Send msg of type %d from %d to %d.\n", msg -> s_header . s_type, pr -> id, dst);
    return 0;
}

int send_multicast(void * self, const Message * msg) {
    struct process* pr = (struct process*) self;
    struct duplex_chanel_list* cur_list;
    local_id receiver_id;

    if (pr == NULL) {
        fprintf(stderr, errmsg_empty_process);
        return 1;
    }
    cur_list = get_pr_chanel_list(pr);

    while (cur_list != NULL) {
        receiver_id = cur_list -> first_id != get_pr_id(pr) ? cur_list -> first_id : cur_list -> second_id;
        if (send(self, receiver_id, msg) != 0) {
            return -1;
        }
        cur_list = cur_list -> next;
    }
    return 0;
}

/**
 * @brief Receive msg from the `from` process
 * 
 * @param self NONNULLABLE struct process
 * @param from sender process local id
 * @param msg NONNULLABLE struct Message
 * @return int `-1` - chanel can not be accessed, system error
 * @return int `0` - msg is receved
 * @return int `1` - chanel with `from` process is empty (NONBLOCK error)
 * @return int `2` - chanel for `from` process not found
 * @return int `3` - argument self (struct process) is NULL  
 * @return int `4` - argument msg (Message) is NULL
 * @return int `6` - msg is inconsistent (Magic is not correct)
 * @return int `7` - chanel is closed, process has exited
 */
int receive(void * self, local_id from, Message * msg) {
    int                                     read_fd;
    struct duplex_chanel_list*              cur_list;
    struct process* pr = (struct process*)  self;

    if (pr == NULL) {
        fprintf(stderr, errmsg_empty_process);
        return 3;
    } 
    if (msg == NULL) {
        fprintf(stderr, "Failed: tried to receive msg to NULL in %d.\n", pr -> id);
        return 4;
    } 
    cur_list = pr -> ch_list;

    read_fd = -1;
    while (cur_list != NULL) {
        if (cur_list -> first_id == from) {
            read_fd = cur_list -> d_ch -> ch2[0];
        } else if (cur_list -> second_id == from) {
            read_fd = cur_list -> d_ch -> ch1[0];
        } else {
            cur_list = cur_list -> next;
            continue;
        }
        break;
    }
    if (read_fd == -1) {
        fprintf(stderr, errmsg_ch_not_found, get_pr_id(pr), from);
        return 2;
    }

    // try to read header (Blocking aproach)
    // May save to some state amount of read bytes to make async
    const ssize_t msg_header_length = sizeof(MessageHeader);
    ssize_t read_header_bytes_total = 0;
    ssize_t read_header_res;
    while (read_header_bytes_total < msg_header_length) {
        read_header_res = read(read_fd, msg, msg_header_length - read_header_bytes_total);
        if (read_header_res == 0) {
            return 7;
        } else if (read_header_res <= -1 && errno == EAGAIN) {
            return 1;
        // add errno handling
        } else if (read_header_res <= -1) {
            return -1;
        }
        // If msg is not put into chanel completely (len returned by read() is less than size of MessageHeader)
        read_header_bytes_total += read_header_res;
    }
    

    if (msg -> s_header.s_magic != MESSAGE_MAGIC) {
        fprintf(stderr, "Failed: msg received from process %d in process %d is not correct\n", from, get_pr_id(pr));
        return 6;
    }
    // If no payload is need to be read
    if (msg -> s_header.s_payload_len == 0) {
        return 0;
    }
    log_msg(msg, true, get_pr_id(pr), from);

    // try to read payload
    const ssize_t msg_payload_length = msg -> s_header.s_payload_len;
    ssize_t read_payload_bytes_total = 0;
    ssize_t read_payload_res;
    while (read_payload_bytes_total < msg_payload_length) {
        read_payload_res = read(read_fd, msg + msg_header_length, msg_payload_length - read_payload_bytes_total);
        // fprintf(stderr, "Pr %d: read %li bytes, %li remainig\n", get_pr_id(self), read_payload_res, msg_payload_length - read_payload_res);
        if (read_payload_res == 0) {
            return 7;
        } else if (read_header_res <= -1 && errno == EAGAIN) {  // Msg is not put into chanel yet
            return 1;
        // handle different errno's
        } else if (read_payload_res <= -1) {
            fprintf(stderr, "Failed to read from fd %d in process %d: %s\n", read_fd, get_pr_id(pr), strerror(errno));
            return -1;
        }
        read_payload_bytes_total += read_payload_res;
    }
    return 0;
}

/**
 * @brief Receive one msg from any process
 * 
 * @param self NONNULLABLE struct process
 * @param msg NONNULLABLE struct Message
 * @return int 0 - Success
 * @return int 1 - process or msg are not valid (NULL)
 * @return int 2 - no msg is sent
 * @return int 3 - magic is broken
 * @return int 4 - chanel is unavaliable, process not opened or finished
 */
int receive_any(void * self, Message * msg) {
    struct process* pr = (struct process*) self;
    struct duplex_chanel_list* cur_list;
    local_id sender_id;
    int ret_val;

    if (pr == NULL) {
        fprintf(stderr, errmsg_empty_process);
        return 1;
    } 
    if (msg == NULL) {
        fprintf(stderr, "Failed: tried to receive msg to NULL in%d.\n", get_pr_id(pr));
        return 1;
    } 
    
    // may select sender randomly
    cur_list = get_pr_chanel_list(pr);
    while (cur_list != NULL) {
        sender_id = cur_list -> first_id != get_pr_id(pr) ? cur_list -> first_id : cur_list -> second_id;
        // avoid receiving from PARENT
        if (sender_id == PARENT_ID) {
            cur_list = cur_list -> next;
            continue;
        }
        switch (receive(self, sender_id, msg)) {
            case -1: {
                fprintf(stderr, "Failed to access chanel from %d to %d, read error: %s\n", sender_id, get_pr_id(self), strerror(errno));
            };
            case 2: {
                fprintf(stderr, "Failed to find open chanel from %d to %d\n", sender_id, get_pr_id(self));
            }
            case 7: {
                // chanel is no longer avaliable, may close it to avoid trying to receive from it again
                // close_duplex_chanel(); {
                // Chanel is broken, check next
                fprintf(stdout, "Failed to receive: Chanel from %d to %d is unavaliable, process might have been finished or terminated\n", sender_id, get_pr_id(self));
                cur_list = cur_list -> next;
                ret_val = 4;
                break;
            };
            case 0: {
                return 0;
            };
            case 1: {
                // printf("Pr %d: no msg is present in chanel with process %d or chanel is full\n", get_pr_id(self), sender_id);
                // Completely async (return if all chanels are empty)
                cur_list = cur_list -> next;
                ret_val = 2;
                break;

                // Completely sync (block until receive any msg)
                // cur_list = cur_list -> next;
                // if (cur_list == NULL) {
                //     cur_list = get_pr_chanel_list(pr);
                // }
                // break;
            };
            case 3:
            case 4: {
                return 1;
            };
            case 6: {
                fprintf(stderr, "Process %d receive from %d: Magic number of received msg is broken\n", get_pr_id(self), sender_id);
                return 3;
            };
            default: {
                return -1;
            };
        }
        // might place assignment of cur = next here 
        fflush(stdout);
    }
    return ret_val;
}

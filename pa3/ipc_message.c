#include "ipc_message.h"
#include "ipc.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Create a mesage object
 * 
 * @param msg_type 
 * @param msg_contents_length 
 * @param msg_payload 
 * @param timestamp 
 * @param msg 
 * @return int 0 - Success
 * @return int 1 - Invalid arguments provided
 * @return int 2 - Failed to allocate memory
 * @return int  
 */
int create_message(const MessageType msg_type, const short msg_payload_len,
                        const void* const msg_payload, timestamp_t timestamp, Message** const msg) {
    if (msg_payload_len > MAX_PAYLOAD_LEN) {
        fprintf(stderr, "Failed to create msg: payload length %d is bigger than max length %d\n", msg_payload_len, MAX_MESSAGE_LEN);
        return 1;
    }
    *msg = malloc(sizeof(MessageHeader) + msg_payload_len);
    if (*msg == NULL) {
        fprintf(stderr, "Failed to allocate memoty for a message\n");
        return 2;
    }
    (*msg) -> s_header = (MessageHeader) {
        .s_magic = MESSAGE_MAGIC,
        .s_type = msg_type,
        .s_payload_len = msg_payload_len,
        .s_local_time = timestamp
    };
    if (msg_payload_len != 0) {
        if (msg_payload == NULL) {
            return 1;
        }
        memcpy((*msg) -> s_payload, msg_payload, msg_payload_len);
    }
    return 0;
}

int create_empty_msg(Message** const msg) {
    *msg = malloc(MAX_MESSAGE_LEN);
    if (*msg == NULL) {
        return 1;
    }
    **msg = (Message) {
        .s_header = (MessageHeader) {
            .s_magic = MESSAGE_MAGIC,
            .s_payload_len = MAX_MESSAGE_LEN,
        },
    };
    return 0;
}

void free_message(Message** const msg) {
    if (*msg != NULL) {
        free(*msg);
    }
}

#include "ipc_message.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int create_mesage(const MessageType msg_type, const short msg_contents_length,
                        const void* const msg_payload, Message** const msg) {
    if (msg_contents_length > MAX_PAYLOAD_LEN) {
        fprintf(stderr, "Failed to create msg: payload length %d is bigger than max length %d\n", msg_contents_length, MAX_MESSAGE_LEN);
        return 1;
    }
    *msg = malloc(sizeof(MessageHeader) + msg_contents_length);
    if (*msg == NULL) {
        fprintf(stderr, "Failed to allocate memoty for a message\n");
        return 2;
    }
    (*msg) -> s_header = (MessageHeader) {
        .s_magic = MESSAGE_MAGIC,
        .s_type = msg_type,
        .s_payload_len = msg_contents_length,
        .s_local_time = 0
    };
    if (msg_contents_length != 0) {
        memcpy((*msg) -> s_payload, msg_payload, msg_contents_length);
    }
    return 0;
}

int create_empty_msg(Message** const msg) {
    *msg = malloc(MAX_MESSAGE_LEN);
    if (*msg == NULL) {
        return 1;
    }
    return 0;
}

void free_message(Message** const msg) {
    if (*msg != NULL) {
        free(*msg);
    }
}

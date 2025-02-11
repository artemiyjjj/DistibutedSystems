#ifndef IPC_MESSAGE_H
#define IPC_MESSAGE_H

#include "ipc.h"

#include <stdbool.h>

static char* MsgTypeStr[CS_RELEASE+1] = {
    [STARTED] = "STARTED",
    [DONE] = "DONE",
    [ACK] = "ACK",
    [STOP] = "STOP",
    [TRANSFER] = "TRANSFER",
    [BALANCE_HISTORY] = "BH",
    [CS_REQUEST] = "CS_REQUEST",
    [CS_REPLY] = "CS_REPLY",
    [CS_RELEASE] = "CS_RELEASE"
};

int create_empty_msg(Message** const msg);

int create_message(const MessageType msg_type, const short msg_contents_length,
                        const void* const msg_contents, timestamp_t timestamp, Message** const msg);

void free_message(Message** const msg);

// void log_msg(const Message* const msg, const bool is_received, const local_id receiver, const local_id sender);

#endif

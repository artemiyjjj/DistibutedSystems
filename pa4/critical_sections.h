#ifndef CRITICAL_SECTION_H
#define CRITICAL_SECTION_H

#include "ipc.h"
#include "process_pub.h"

bool is_proc_allowed_cs(struct process* self);


// int process_request_cs(struct process* self);

// int process_release_cs(struct process* self);

int process_reply_cs(struct process* self, const local_id dest);



void handle_cs_request(struct process* self, Message* msg, local_id from);

void handle_cs_reply(struct process* self, Message* msg, local_id from);

void handle_cs_release(struct process* self, Message* msg, local_id from);

#endif

/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#include <cstdlib>
#include "muxqueue.h"

namespace Ms {

struct MuxQueue * mux_mq_new (int mailboxSize) {
    struct MuxQueue *q = (struct MuxQueue *) malloc(sizeof(struct MuxQueue));
    q->reader = 0;
    q->writer = 0;
    q->mailboxSize = mailboxSize;
    q->mailbox = (struct MuxQueueItem *) malloc(sizeof(struct MuxQueueItem) * mailboxSize);
    return q;
}

void mux_mq_free (struct MuxQueue *q) {
    free(q->mailbox);
    free(q);
}

void* mux_mq_read (struct MuxQueue *queue) {
    int reader = queue->reader;
    int writer = queue->writer;
    if (reader == writer) {
        return 0;
    }
    struct MuxQueueItem *qi = &queue->mailbox[reader];
    void* data = qi->data;
    queue->reader = (reader + 1) % queue->mailboxSize;
    return data;
}

int mux_mq_write (struct MuxQueue *queue, void* data) {
    int reader = queue->reader;
    int writer = queue->writer;
    int newPos  = (writer + 1) % queue->mailboxSize;
    if (newPos == reader) return -1; // mailbox is full
    struct MuxQueueItem *qi = &queue->mailbox[writer];
    qi->type = 0;
    qi->label[0] = 0;
    qi->data = data;
    queue->writer = newPos;
    return 0;
}

} // namespace Ms

/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */
#ifndef __MUXQUEUE_H__
#define __MUXQUEUE_H__

#include <cstdint>

namespace Ms {

struct MuxQueueItem {
    uint32_t type;
    char label[64];
    void *data;
};

struct MuxQueue {
    int reader;
    int writer;
    int mailboxSize;
    struct MuxQueueItem *mailbox;
};

struct MuxQueue * mux_mq_new (int mailboxSize);
void mux_mq_free (struct MuxQueue *q);
void* mux_mq_read (struct MuxQueue *queue);
int mux_mq_write (struct MuxQueue *queue, void* data);

} // namespace Ms
#endif

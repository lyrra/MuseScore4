/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */
/*
*/


#include "queue.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace Mux {

/*
 * message queue, between audio and mux
 */ 

#define MAILBOX_SIZE 256
unsigned int g_msg_from_audio[MAILBOX_SIZE];
int g_msg_from_audio_reader = 0;
int g_msg_from_audio_writer = 0;

#if 0
// audio-thread uses this function to send messages to mux/mscore
int mux_mq_from_audio_writer_put (struct Msg msg) {
    memcpy(&g_msg_from_audio[g_msg_from_audio_writer].payload, &msg.payload, sizeof(msg.payload));
    // setting the type will signal to the reader that this slot is filled
    g_msg_from_audio[g_msg_from_audio_writer].type = msg.type;
    g_msg_from_audio_writer = (g_msg_from_audio_writer + 1) % MAILBOX_SIZE;
    return 1;
}
#endif

#if 0
int mux_mq_from_audio_reader_visit () {
    if (g_msg_from_audio_reader == g_msg_from_audio_writer) {
        return 0;
    }
    Msg msg = g_msg_from_audio[g_msg_from_audio_reader];
    int rc = mux_mq_from_muxaudio_handle(msg);
    g_msg_from_audio_reader = (g_msg_from_audio_reader + 1) % MAILBOX_SIZE;
    return rc;
}

/* message to/from audio helpers
 */
void mux_msg_to_audio(MsgType typ, int val)
{
    // put message on MQ towards audio thread (second part of mux)
    struct Msg msg;
    msg.type = typ;
    msg.payload.i = val;
    mux_zmq_ctrl_send_to_audio(msg);
}

void mux_msg_from_audio(MsgType typ, int val)
{
    struct Msg msg;
    msg.type = typ;
    msg.payload.i = val;
    mux_mq_from_audio_writer_put(msg);
}

void msgToAudioSeekTransport(int utick) {
    mux_msg_to_audio(MsgTypeTransportSeek, utick);
}

/*
 * Audio ringbuffer from mux to audio
 */
#define MUX_CHAN 2
#define MUX_RINGSIZE (8192*2)
#define MUX_CHUNKSIZE (2048*2)
#define MUX_READER_USLEEP 100
#define MUX_WRITER_USLEEP 100

unsigned int g_ringBufferWriterStart = 0;
unsigned int g_ringBufferReaderStart = 0;
float g_ringBufferStereo[MUX_RINGSIZE];
// minimal amount of work considered feasible (performance-wise)
float g_chunkBufferStereo[MUX_CHUNKSIZE];

unsigned int g_readerCycle = 0;
unsigned int g_writerCycle = 0;
unsigned int g_readerPause = 0;
unsigned int g_writerPause = 0;

// this function is called by the zmq-network audio connection towards muxaudio
void mux_process_bufferStereo(unsigned int numFloats, float* frames) {
    while (1) {
        unsigned int newReaderPos = (g_ringBufferReaderStart + numFloats) % MUX_RINGSIZE;
        // ensure we dont read into writers buffer part
        if (// if reader wraps around and goes beyond writer
            (g_ringBufferReaderStart > newReaderPos &&
             newReaderPos > g_ringBufferWriterStart) ||
            // no wrap, writer is at right side of reader, but new reader goes beyond writer
            (g_ringBufferWriterStart > g_ringBufferReaderStart &&
             newReaderPos > g_ringBufferWriterStart)) {
            g_readerPause++;
            std::this_thread::sleep_for(std::chrono::microseconds(MUX_READER_USLEEP));
        } else { // there is enough room in reader-part of ring-buffer to use
            for (unsigned int i = 0; i < numFloats; i++) {
                frames[i] =
                 g_ringBufferStereo[(i + g_ringBufferReaderStart) % MUX_RINGSIZE];
            }
            if (newReaderPos < g_ringBufferReaderStart) {
                g_readerCycle++;
            }
            g_ringBufferReaderStart = newReaderPos;
            break;
        }
    }
}
#endif

int mux_audio_process_work() {
#if 0
    unsigned int newWriterPos = (g_ringBufferWriterStart + MUX_CHUNKSIZE) % MUX_RINGSIZE;

    // ensure we dont overwrite part of buffer that is being read by reader
    // if writer wraps around and goes beyond reader
    if (g_ringBufferReaderStart < g_ringBufferWriterStart &&
        g_ringBufferWriterStart > newWriterPos &&
        newWriterPos >= g_ringBufferReaderStart) {
        return 0;
    // wraps around, but goes beyond reader
    } else if (g_ringBufferWriterStart < g_ringBufferReaderStart &&
               newWriterPos < g_ringBufferWriterStart) {
        return 0;
    // no wrap, reader is at right side of writer, but new writer goes beyond reader
    } else if (g_ringBufferWriterStart < g_ringBufferReaderStart &&
               newWriterPos >= g_ringBufferReaderStart) {
        return 0;
    }

    // process a MUX_CHUNKSIZE number of Frames
    memset(g_chunkBufferStereo, 0, sizeof(float) * MUX_CHUNKSIZE);
    // fill the chunk with audio content
    seq->process(MUX_CHUNKSIZE >> 1, g_chunkBufferStereo);
    // copy over the chunk to the ringbuffer
    for (unsigned int i = 0; i < MUX_CHUNKSIZE; i++) {
        g_ringBufferStereo[(i + g_ringBufferWriterStart) % (MUX_RINGSIZE)] =
         g_chunkBufferStereo[i];
    }
    if (newWriterPos < g_ringBufferWriterStart) {
        g_writerCycle++;
    }
    /*

    std::cout << "writer: [" << g_ringBufferWriterStart << " - " << newWriterPos << "] "
              << "r/w: [" << g_ringBufferReaderStart << " - " << g_ringBufferWriterStart << "]"
              << " c:[" << g_readerCycle << ", " << g_writerCycle << "] "
              << " p:[" << g_readerPause << ", " << g_writerPause << "]\n";
    */
    g_ringBufferWriterStart = newWriterPos;
#endif
    return 1;
}

#if 0
void mux_audio_process() {
    int slept = MUX_WRITER_USLEEP; // we cant sleep longer than the jack-audio-period = (numFrames / SampleRate) seconds
    while (mux_audio_process_run) {
        if ((! mux_audio_process_work()) && // no audio work was done,
            (! mux_mq_from_audio_reader_visit())) { // and message-queue is empty
            g_writerPause++;
            std::this_thread::sleep_for(std::chrono::microseconds(slept));
        }
    }
    qDebug("MUX audio-process terminated.");
}
#endif


} // end of namespace Mux

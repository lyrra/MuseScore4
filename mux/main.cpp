/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <zmq.h>
#include <thread>
#include <chrono>

namespace Ms {

#define MUX_SYNC_MSLEEP 100

//void muxseq_threads_start();

//void muxseq_send_event_to_gui(struct SparseEvent se);
//void muxseq_audio_send_event_to_midi(struct Msg msg);

//static std::vector<std::thread> seqThreads;
//int mux_audio_process_run = 0;

static void *zmq_context_seq;
static void *zmq_socket_seq;

/*
 * message queue, between audio and mux
 */ 

//#define MAILBOX_SIZE 256
//struct Msg g_msg_to_audio[MAILBOX_SIZE];
//struct Msg g_msg_from_audio[MAILBOX_SIZE];
//int g_msg_to_audio_reader = 0;
//int g_msg_to_audio_writer = 0;
//int g_msg_from_audio_reader = 0;
//int g_msg_from_audio_writer = 0;

#if 0
// audio-thread uses this function to send messages to mux/mscore
int muxseq_mq_from_audio_writer_put (struct Msg msg) {
    memcpy(&g_msg_from_audio[g_msg_from_audio_writer].payload, &msg.payload, sizeof(msg.payload));
    // setting the type will signal to the reader that this slot is filled
    g_msg_from_audio[g_msg_from_audio_writer].type = msg.type;
    g_msg_from_audio_writer = (g_msg_from_audio_writer + 1) % MAILBOX_SIZE;
    return 1;
}

// mux/mscore thread uses this function to send messages to audio-thread
int muxseq_mq_to_audio_writer_put (struct Msg msg) {
    memcpy(&g_msg_to_audio[g_msg_to_audio_writer].payload, &msg.payload, sizeof(msg.payload));
    // setting the type will signal to the reader that this slot is filled
    g_msg_to_audio[g_msg_to_audio_writer].type = msg.type;
    g_msg_to_audio_writer = (g_msg_to_audio_writer + 1) % MAILBOX_SIZE;
    return 1;
}

int muxseq_mq_from_audio_reader_visit () {
    if (g_msg_from_audio_reader == g_msg_from_audio_writer) {
        return 0;
    }
    Msg msg = g_msg_from_audio[g_msg_from_audio_reader];
    switch (msg.type) {
        case MsgTypeAudioRunning:
            //g_driver_running = msg.payload.i;
            // FIX: send to seq (over network)
            std::cout << "g_driver_running is running? " << msg.payload.i << "\n";
            if (zmq_send(zmq_socket_ctrl, &msg, sizeof(struct Msg), 0) < 0) {
                // musescore cant start if it gets no go-ahead signal
                std::cerr << "failed to tell musescore that muxaudio-driver is in running state.\n";
            }
        break;
        case MsgTypeJackTransportPosition:
            if (zmq_send(zmq_socket_ctrl, &msg, sizeof(struct Msg), 0) < 0) {
                std::cerr << "failed to tell musescore current jack position/state\n";
            }
        break;
        case MsgTypeEventToGui:
            //mux_send_event_to_gui(msg.payload.sparseEvent);
            // FIX: send to seq (over network)
        break;
        default: // this should not happen
            std::cerr << "MUX got unknown message from audio: " << msg.type << "\n";
            // skip this message
            g_msg_from_audio_reader = (g_msg_from_audio_reader + 1) % MAILBOX_SIZE;
        return 0;
    }
    msg.type = MsgTypeInit; // mark this as free, FIX: not needed
    g_msg_from_audio_reader = (g_msg_from_audio_reader + 1) % MAILBOX_SIZE;
    return 1;
}

int muxseq_mq_to_audio_visit() {
    if (g_msg_to_audio_reader == g_msg_to_audio_writer) {
        return 0;
    }
    struct Msg msg = g_msg_to_audio[g_msg_to_audio_reader];
    std::cerr << "MUX ctrl message, type: " << msg.type << " " << mux_msg_type_name(msg.type) << "\n";
    switch (msg.type) {
        case MsgTypeAudioInit:
            mux_audio_init(msg.payload.i);
        break;
        case MsgTypeAudioStart:
            mux_audio_start(msg.payload.i);
        break;
        case MsgTypeAudioStop:
            mux_audio_stop();
        break;
        case MsgTypeTransportStart:
            mux_audio_jack_transport_start();
        break;
        case MsgTypeTransportStop:
            mux_audio_jack_transport_stop();
        break;
        case MsgTypeTransportSeek:
            mux_audio_jack_transport_seek(msg.payload.i);
        break;
        case MsgTypeEventToMidi:
            mux_audio_send_event_to_midi(msg);
        break;
        case MsgTypeTimeSigTempoChanged:
            mux_audio_handle_MsgTimeSigTempoChanged();
        break;
        case MsgTypeOutPortCount:
            mux_audio_handle_updateOutPortCount(msg.payload.i);
        break;
        default: // this should not happen
            std::cout << "MUX got unknown message from audio: " << msg.type << "\n";
            // skip this message
            g_msg_to_audio_reader = (g_msg_to_audio_reader + 1) % MAILBOX_SIZE;
        return 0;
    }
    msg.type = MsgTypeInit; // mark this as free, FIX: not needed
    g_msg_to_audio_reader = (g_msg_to_audio_reader + 1) % MAILBOX_SIZE;
    //std::cout << "mux_to_audio new r:" << g_msg_to_audio_reader << "\n";
    return 1;
}

/* message to/from audio helpers
 */
void muxseq_msg_to_audio(MsgType typ, int val)
{
    struct Msg msg;
    msg.type = typ;
    msg.payload.i = val;
    mux_mq_to_audio_writer_put(msg);
}

void muxseq_msg_from_audio(MsgType typ, int val)
{
    struct Msg msg;
    msg.type = typ;
    msg.payload.i = val;
    mux_mq_from_audio_writer_put(msg);
}
#endif

#if 0
/*
 * Audio ringbuffer from mux to audio
 */
#define MUX_CHAN 2
#define MUX_RINGSIZE (8192*2)
#define MUX_CHUNKSIZE (2048*2)
#define MUX_READER_USLEEP 10
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


// this function is called by the realtime-context,
// and is reading from the ring-buffer
void muxseq_process_bufferStereo(unsigned int numFrames, float* bufferStereo){
    // we're in realtime-context, and shouldn't do any syscalls,
    // or sleep because there is no maximum sleep-amount guarantees,
    // if paranoid, just spin-loop
    while (1) {
        unsigned int newReaderPos = (g_ringBufferReaderStart + numFrames * MUX_CHAN) % MUX_RINGSIZE;
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
            for (unsigned int i = 0; i < numFrames * MUX_CHAN; i++) {
                bufferStereo[i] =
                 g_ringBufferStereo[(i + g_ringBufferReaderStart) % MUX_RINGSIZE];
            }
            if (newReaderPos < g_ringBufferReaderStart) {
                g_readerCycle++;
            }
            /*
            std::cout << "reader: [" << g_ringBufferReaderStart << " - " << newReaderPos << "]"
                      << "r/w: [" << g_ringBufferReaderStart << " - " << g_ringBufferWriterStart << "]"
                      << " c:[" << g_readerCycle << ", " << g_writerCycle << "] "
                      << " p:[" << g_readerPause << ", " << g_writerPause << "]\n";
            */
            g_ringBufferReaderStart = newReaderPos;
            break;
        }
    }
}

// this is the non-realtime part, and is requested to do work
// by the realtime-part, and then buffering its work-content
// and is writing to the ring-buffer
int muxseq_audio_process_work() {
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
    //memset(g_chunkBufferStereo, 0, sizeof(float) * MUX_CHUNKSIZE);
    // Request a chunk over the network from mscore/seq
    //seq->process(MUX_CHUNKSIZE >> 1, g_chunkBufferStereo);
    if (zmq_send(zmq_socket_audio, "a", 1, 0) < 0) return -1;
    if (zmq_recv(zmq_socket_audio, g_chunkBufferStereo, sizeof(float) * MUX_CHUNKSIZE, 0) < 0) return -1;
    //zmq_recv(zmq_socket_audio, g_chunkBufferStereo, 1, 0);
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
    return 1;
}

void muxseq_audio_process() {
    int slept = MUX_WRITER_USLEEP; // we cant sleep longer than the jack-audio-period = (numFrames / SampleRate) seconds
    while (mux_audio_process_run) {
        int rc = mux_audio_process_work();
        if (rc < 0) { // error, sleep longer
            std::cout << "MUX mscore/seq audio not ready, waiting..\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(MUX_SYNC_MSLEEP));
        } else {
            if ((! mux_audio_process_work()) && // no audio work was done,
                (! mux_mq_from_audio_reader_visit())) { // and message-queue is empty
                g_writerPause++;
                std::this_thread::sleep_for(std::chrono::microseconds(slept));
            }
        }
    }
    std::cout << "MUX audio-process terminated.\n";
}
#endif

#if 0
void muxseq_network_mainloop_ctrl()
{
    std::cerr << "MUX ZeroMQ control entering main-loop\n";
    while (1) {
        struct Msg msg;
        if (zmq_recv(zmq_socket_ctrl, &msg, sizeof(struct Msg), 0) < 0) {
            fprintf(stderr, "zmq-recv control  error: %s\n", strerror(errno));
            break;
        }
        fprintf(stderr, "Received Control Message, type=(%i)%s\n", msg.type,
                mux_msg_type_name(msg.type));
        mux_mq_to_audio_writer_put(msg);
    }
    fprintf(stderr, "mux_network_mainloop control has exited\n");
}


void muxseq_network_mainloop_audio()
{
    std::cerr << "MUX ZeroMQ audio entering main-loop\n";
    while (1) {
        std::this_thread::sleep_for(std::chrono::microseconds(100000));
    }
    fprintf(stderr, "mux_network_mainloop audio has exited\n");
}
#endif

void muxseq_network_server_ctrl()
{
    //mux_network_open_ctrl();
    //mux_network_mainloop_ctrl();
}

void muxseq_network_server_audio()
{
    //mux_network_open_audio();
    //mux_network_mainloop_audio();
}

} // end of namespace MS

int main(int argc, char **argv)
{
    //Ms::muxseq_threads_start();
    while(1){
        std::this_thread::sleep_for(std::chrono::microseconds(100000));
    }
    return 0;
}

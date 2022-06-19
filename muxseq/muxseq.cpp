/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */
/*
 
    audio: the real-time thread that is governed by JACK
    mux: a thread that sits between musescore and JACK
    mscore: musescore main thread

    audio <--> mux <---> mscore

    The mux-thread will work ahead of the audio-thread
    to process score-events into actual audiobuffers.

               +---------------------------+
    audio <--- | g_ringBufferStereo        |
               |   g_ringBufferReaderStart | reader is audio-thread
               |   g_ringBufferWriterStart | writer is mux-thread
               +---------------------------+
                 <--- mux
                      g_chunkBufferStereo (intermediate audiobuffer)
                      <--- mscore
        1) mux calls into mscore synthesizers (Seq::process), to
           fill the audiobuffer in g_chunkBufferStereo.
           This is synchronous and needs to ringBuffer synchronization.
        2) mux will synchronize with the ringbuffer and write
           the g_chunkBufferStereo audiobuffer into it.
        3) the audio thread detects that the ringbuffer-writer has
           advanced and can read out audio into it own buffers.
    The writer will look at the ringbuffer, if enough free room is
    available to fill the size of a chunk (g_chunkBufferStereo),
    it will call mscore-synthesizers to process a chunck.
    Statistics about the ringbuffer activity is kept in four variables:
      - g_readerCycle  -- everytime the reader wraps around the buffer
      - g_writerCycle  -- everytime the writer wraps around the buffer
      - g_readerPause  -- reader has no audio to read, it will sleep
      - g_writerPause  -- writer has no audio to write, it will sleep

    Aside from feeding jack with audiobuffers, audio and mux thread
    need to signal information, for example transport play/stop.
    This is done by sending message directly using ZeroMQ.

    ..--------+                  +------------..
     muxaudio | <-- ZeroMQ ----> | mscore/seq
    ..-------+|                  +------------..

*/


#include "config.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <zmq.h>
#include "event.h"
#include "mux.h"
#include "muxcommon.h"
#include "muxlib.h"
#include "muxseq.h"
#include "seq.h"

namespace Ms {

void mux_zmq_ctrl_send_to_audio(struct MuxaudioMsg msg);
void mux_send_event_to_gui(struct SparseEvent se);
void mux_audio_send_event_to_midi(struct MuxaudioMsg msg);
extern int g_driver_running;

static std::vector<std::thread> seqThreads;
int g_muxseq_audio_process_run = 0;

static void *zmq_context_ctrl;
static void *zmq_socket_ctrl;
static void *zmq_context_audio;
static void *zmq_socket_audio;

extern Seq* g_seq;

void seq_create(int sampleRate); // FIX: move into seq.h

/*
 * message queue, between audio and mux
 */ 

#define MAILBOX_SIZE 256
struct MuxaudioMsg g_msg_from_audio[MAILBOX_SIZE];
int g_msg_from_audio_reader = 0;
int g_msg_from_audio_writer = 0;

// audio-thread uses this function to send messages to mux/mscore
int mux_mq_from_audio_writer_put (struct MuxaudioMsg msg) {
    memcpy(&g_msg_from_audio[g_msg_from_audio_writer].payload, &msg.payload, sizeof(msg.payload));
    // setting the type will signal to the reader that this slot is filled
    g_msg_from_audio[g_msg_from_audio_writer].type = msg.type;
    g_msg_from_audio_writer = (g_msg_from_audio_writer + 1) % MAILBOX_SIZE;
    return 1;
}

int mux_mq_from_audio_reader_visit () {
    if (g_msg_from_audio_reader == g_msg_from_audio_writer) {
        return 0;
    }
    struct MuxaudioMsg msg = g_msg_from_audio[g_msg_from_audio_reader];
    int rc = 0; //FIX muxseq_mq_from_muxaudio_handle(msg);
    g_msg_from_audio_reader = (g_msg_from_audio_reader + 1) % MAILBOX_SIZE;
    return rc;
}

/* message to/from audio helpers
 */
void muxseq_msg_to_audio(MuxaudioMsgType typ, int val)
{
    // put message on MQ towards audio thread (second part of mux)
    struct MuxaudioMsg msg;
    msg.type = typ;
    msg.payload.i = val;
    mux_zmq_ctrl_send_to_audio(msg);
}

void mux_msg_from_audio(MuxaudioMsgType typ, int val)
{
    struct MuxaudioMsg msg;
    msg.type = typ;
    msg.payload.i = val;
    mux_mq_from_audio_writer_put(msg);
}

void msgToAudioSeekTransport(int utick) {
    muxseq_msg_to_audio(MsgTypeTransportSeek, utick);
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
    memset(g_chunkBufferStereo, 0, sizeof(float) * MUX_CHUNKSIZE);
    // fill the chunk with audio content
    g_seq->process(MUX_CHUNKSIZE >> 1, g_chunkBufferStereo);
    // copy over the chunk to the ringbuffer
    for (unsigned int i = 0; i < MUX_CHUNKSIZE; i++) {
        g_ringBufferStereo[(i + g_ringBufferWriterStart) % (MUX_RINGSIZE)] =
         g_chunkBufferStereo[i];
    }
    if (newWriterPos < g_ringBufferWriterStart) {
        g_writerCycle++;
    }
    g_ringBufferWriterStart = newWriterPos;
    return 1;
}

void muxseq_muxaudioWorker_process() {
    int slept = MUX_WRITER_USLEEP; // we cant sleep longer than the jack-audio-period = (numFrames / SampleRate) seconds
    while (g_muxseq_audio_process_run) {
        if ((! muxseq_audio_process_work()) && // no audio work was done,
            (! mux_mq_from_audio_reader_visit())) { // and message-queue is empty
            g_writerPause++;
            std::this_thread::sleep_for(std::chrono::microseconds(slept));
        }
    }
    LD("MUXSEQ audio-process terminated.");
}

void mux_zmq_ctrl_send_to_audio (struct MuxaudioMsg msg)
{
    qDebug("zmq-send ctrl msg.type=%i (len %i)", msg.type, sizeof(struct MuxaudioMsg));
    zmq_send(zmq_socket_ctrl, &msg, sizeof(struct MuxaudioMsg), 0);
}

int muxseq_handle_musescore_reply_int (Mux::MuxSocket &sock, struct MuxseqMsg msg, int i) {
    msg.payload.i = i;
    return zmq_send(sock.socket, &msg, sizeof(struct MuxseqMsg), 0);
}

int muxseq_handle_musescore_msg_SeqAlive (Mux::MuxSocket &sock, struct MuxseqMsg msg) {
    return muxseq_handle_musescore_reply_int(sock, msg, 1); // always alive
}

int muxseq_handle_musescore_msg_SeqRunning (Mux::MuxSocket &sock, struct MuxseqMsg msg) {
    LD("got message from musescore, SeqRunning=%i", g_driver_running);
    return muxseq_handle_musescore_reply_int(sock, msg, g_driver_running);
}

int muxseq_handle_musescore_msg_SeqPreferencesChanged (Mux::MuxSocket &sock, struct MuxseqMsg msg) {
    // FIX: copy preferences?
    return muxseq_handle_musescore_reply_int(sock, msg, 0);
}

int muxseq_handle_musescore_msg_SeqCreate (Mux::MuxSocket &sock, struct MuxseqMsg msg) {
    // FIX: initialize seq
    LD("got message from musescore, create Sequencer, sampleRate=%i", msg.payload.i);
    seq_create(msg.payload.i);
    return muxseq_handle_musescore_reply_int(sock, msg, 0);
}

int muxseq_handle_musescore_msg_SeqInit (Mux::MuxSocket &sock, struct MuxseqMsg msg) {
    // FIX: initialize seq
    LD("got message from musescore, init Sequencer, hotPlug=%i", msg.payload.b);
    return muxseq_handle_musescore_reply_int(sock, msg, 0);
}

int muxseq_handle_musescore_msg_SeqStopNoteTimer (Mux::MuxSocket &sock, struct MuxseqMsg msg) {
    // FIX: initialize seq
    return muxseq_handle_musescore_reply_int(sock, msg, 0);
}

int muxseq_handle_musescore_msg_SeqExit (Mux::MuxSocket &sock, struct MuxseqMsg msg) {
    // FIX: destroy seq
    return muxseq_handle_musescore_reply_int(sock, msg, 0);
}

int muxseq_handle_musescore_msg(Mux::MuxSocket &sock, struct MuxseqMsg msg)
{
    LD("got message from musescore: %s", muxseq_msg_type_info(msg.type));
    switch (msg.type) {
        case MsgTypeSeqAlive:
            return muxseq_handle_musescore_msg_SeqAlive(sock, msg);
        case MsgTypeSeqRunning:
            return muxseq_handle_musescore_msg_SeqRunning(sock, msg);
        case MsgTypeSeqPreferencesChanged:
            return muxseq_handle_musescore_msg_SeqPreferencesChanged(sock, msg);
        case MsgTypeSeqCreate:
            return muxseq_handle_musescore_msg_SeqCreate(sock, msg);
        case MsgTypeSeqInit:
            return muxseq_handle_musescore_msg_SeqInit(sock, msg);
        case MsgTypeSeqStopNoteTimer:
            return muxseq_handle_musescore_msg_SeqStopNoteTimer(sock, msg);
        case MsgTypeSeqExit:
            return muxseq_handle_musescore_msg_SeqExit(sock, msg);
        default:
            LD("ERROR: Unknown message: %s", muxseq_msg_type_info(msg.type));
        break;
    }
    return 0;
}

int muxseq_handle_muxaudioQueryClient_msg_AudioBufferFeed (Mux::MuxSocket &sock, struct MuxaudioMsg msg)
{
    // get MUX_CHUNKSIZE number of frames from the ringbuffer
    float frames[MUX_CHUNKSIZE]; // count stereo, ie number of floats needed
    mux_process_bufferStereo(MUX_CHUNKSIZE, frames);
    zmq_send(sock.socket, frames, sizeof(float) * MUX_CHUNKSIZE, 0);
    return 0;
}

int muxseq_handle_muxaudioQueryClient_msg (Mux::MuxSocket &sock, struct MuxaudioMsg msg)
{
    switch (msg.type) {
        case MsgTypeAudioBufferFeed:
            return muxseq_handle_muxaudioQueryClient_msg_AudioBufferFeed(sock, msg);
        //FIX: perhaps not all these can be done from this thread?
        case MsgTypeAudioRunning:
            g_driver_running = msg.payload.i;
            LD("---- g_driver_running is running? %i", msg.payload.i);
        break;
        case MsgTypeJackTransportPosition:
            mux_set_jack_position(msg.payload.jackTransportPosition);
        break;
        case MsgTypeEventToGui:
            mux_send_event_to_gui(msg.payload.sparseEvent);
        break;
        default: // this should not happen
            LD("audio-query-client WARNING: unknown msg from muxaudio: %i", msg.type);
            // skip this message
            // g_msg_from_audio_reader = (g_msg_from_audio_reader + 1) % MAILBOX_SIZE;
    }
    msg.type = MsgTypeAudioNoop;
    zmq_send(sock.socket, &msg, sizeof(struct MuxaudioMsg), 0);
    return 0;
}


/* muxaudioQueryServer -- handle requests from muxaudio
 *
 */
void muxseq_muxaudioQueryClient_mainloop(Mux::MuxSocket &sock)
{
    LD("MUXSEQ muxseq <== muxaudio -- query client started");
    while (1) {
        struct MuxaudioMsg msg;
        if (zmq_recv(sock.socket, &msg, sizeof(struct MuxaudioMsg), 0) < 0) {
            LE("audioQueryClient zmq-recv error: %s", strerror(errno));
            break;
        }
        muxseq_handle_muxaudioQueryClient_msg(sock, msg);
    }
    LD("mux_network_mainloop audio has exited");
}

/* mscoreQueryServer -- handle requests from musescore
 *
 */
void muxseq_mscoreQueryServer_mainloop(Mux::MuxSocket &sock) {
    LD("MUXSEQ musescore ==> muxseq -- query server started");
    while(1){
        struct MuxseqMsg msg;
        if (zmq_recv(sock.socket, &msg, sizeof(struct MuxseqMsg), 0) < 0) {
            LE("mscoreQueryServer zmq-recv error: %s", strerror(errno));
            break;
        }
        muxseq_handle_musescore_msg(sock, msg);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

} // end of namespace MS

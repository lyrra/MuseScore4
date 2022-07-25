/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */
/*
 
    audio: the real-time thread that is governed by JACK
    mux: a thread that sits between musescore and JACK
    mscore: musescore main thread
    
                 muxaudio executable
                +-----------------------------+
                |                             |
    JACK <--->  | audio  <--> ringbuffer <--->|<------ ZeroMQ ---> mscore
                | thread      Zmq-recv-thread |
                +-----------------------------+

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
    This is done by a simple message-queue also in form of a ringbuffer.

               +-------------------------+
    audio <--- | g_msg_to_audio          | <--- mux
               |   g_msg_to_audio_reader |
               |   g_msg_to_audio_writer |
               +-------------------------+

               +---------------------------+
    audio ---> | g_msg_from_audio          | ---> mux
               |   g_msg_from_audio_reader |
               |   g_msg_from_audio_writer |
               +---------------------------+
*/


#include "config.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <zmq.h>
#include "mux.h"
#include "muxcommon.h"
#include "event.h"
#include "muxlib.h"
#include "muxaudio.h"
#include "driver.h"

namespace Ms {

void _logstr (char *str) {
    char *nl = "\n";
    fwrite(str, strlen(str), 1, stdout);
    fwrite(nl, 1, 1, stdout);
    fflush(stdout);
}

#define MUX_SYNC_MSLEEP 100


Driver* g_driver;

void mux_send_event_to_gui(struct SparseEvent se);
void mux_audio_send_event_to_midi(struct MuxaudioMsg msg);
//extern int g_driver_running;

static std::vector<std::thread> seqThreads;
int g_mux_audio_process_run = 0;
bool g_state_play = false;

struct Mux::MuxSocket g_socket_ctrl;
struct Mux::MuxSocket g_socket_audio;

//FIX: move to muxlib
void mux_send_event (Event e) {
    struct SparseEvent se;
    se.type    = e.type();
    se.channel = e.channel();
    se.pitch   = e.pitch();
    se.velo    = e.velo();
    struct MuxaudioMsg msg;
    msg.type = MsgTypeEventToGui;
    memcpy(&msg.payload.sparseEvent, &se, sizeof(struct SparseEvent));
    muxaudio_mq_from_audio_writer_put(msg);
}

void mux_audio_init(int hot)
{
    g_driver->init(hot);
}

void mux_audio_start(int hotPlug)
{
    g_driver->start(hotPlug);
}

void mux_audio_stop()
{
}

void mux_audio_jack_transport_start() {
    qDebug("--- mux_audio_jack_transport_start\n");
    g_driver->startTransport();
}

void mux_audio_jack_transport_stop() {
    g_driver->stopTransport();
}

void mux_audio_jack_transport_seek(int utick) {
    g_driver->seekTransport(utick);
}

void mux_audio_handle_MsgTimeSigTempoChanged()
{
    g_driver->handleTimeSigTempoChanged();
}

void mux_audio_handle_updateOutPortCount(int portCount)
{
    g_driver->updateOutPortCount(portCount);
}

void mux_audio_send_event_to_midi(struct MuxaudioMsg msg) {
    NPlayEvent event;
    event.setType(msg.payload.sparseMidiEvent.type);
    event.setDataA(msg.payload.sparseMidiEvent.dataA);
    event.setDataB(msg.payload.sparseMidiEvent.dataB);
    g_driver->putEvent(event, msg.payload.sparseMidiEvent.framepos,
                              msg.payload.sparseMidiEvent.midiPort,
                              msg.payload.sparseMidiEvent.channel);
}

/*
 * message queue, between audio and mux
 */ 

#define MAILBOX_SIZE 256
struct MuxaudioMsg g_msg_to_audio[MAILBOX_SIZE];
struct MuxaudioMsg g_msg_from_audio[MAILBOX_SIZE];
int g_msg_to_audio_reader = 0;
int g_msg_to_audio_writer = 0;
int g_msg_from_audio_reader = 0;
int g_msg_from_audio_writer = 0;

// audio-thread uses this function to send messages to mux/mscore
int muxaudio_mq_from_audio_writer_put (struct MuxaudioMsg msg) {
    memcpy(&g_msg_from_audio[g_msg_from_audio_writer].payload, &msg.payload, sizeof(msg.payload));
    // setting the type will signal to the reader that this slot is filled
    g_msg_from_audio[g_msg_from_audio_writer].type = msg.type;
    g_msg_from_audio_writer = (g_msg_from_audio_writer + 1) % MAILBOX_SIZE;
    return 1;
}

// mux/mscore thread uses this function to send messages to audio-thread
int mux_mq_to_audio_writer_put (struct MuxaudioMsg msg) {
    memcpy(&g_msg_to_audio[g_msg_to_audio_writer].payload, &msg.payload, sizeof(msg.payload));
    // setting the type will signal to the reader that this slot is filled
    g_msg_to_audio[g_msg_to_audio_writer].type = msg.type;
    g_msg_to_audio_writer = (g_msg_to_audio_writer + 1) % MAILBOX_SIZE;
    return 1;
}


int muxaudio_from_audio_reply (struct MuxaudioMsg msg) {
    //FIX: handle error
    mux_request(g_socket_audio, &msg, sizeof(struct MuxaudioMsg));
    return 0;
}

/* FIX: this doesn't do proper zmq-query, use PUB/SUB instead? */
int muxaudio_from_audio_handle_message(struct MuxaudioMsg msg) {
    //LD("muxaudio_from_audio_handle_message msg: %s\n", muxaudio_msg_type_info(msg.type));
    switch (msg.type) {
        case MsgTypeAudioRunning:
            LD("    MsgTypeAudioRunning: g_driver_running is running? %i", msg.payload.i);
            muxaudio_from_audio_reply(msg);
        break;
        case MsgTypeJackTransportPosition:
            muxaudio_from_audio_reply(msg);
        break;
        case MsgTypeEventToGui:
            //mux_send_event_to_gui(msg.payload.sparseEvent);
            // FIX: send to seq (over network)
        break;
        default: // this should not happen
            LE("MUX got unknown message from audio: %s", muxaudio_msg_type_info(msg.type));
        return 0;
    }
    return 1;
}

int muxaudio_mq_from_audio_reader_visit () {
    if (g_msg_from_audio_reader == g_msg_from_audio_writer) {
        return 0;
    }
    struct MuxaudioMsg msg = g_msg_from_audio[g_msg_from_audio_reader];
    muxaudio_from_audio_handle_message(msg);
    msg.type = MsgTypeInit; // mark this as free, FIX: not needed
    g_msg_from_audio_reader = (g_msg_from_audio_reader + 1) % MAILBOX_SIZE;
    return 1;
}

int muxaudio_mq_to_audio_handle_message(struct MuxaudioMsg msg) {
    LD("MUX ctrl message, type: %i(%s)", msg.type, muxaudio_msg_type_info(msg.type));
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
            g_state_play = true;
            mux_audio_jack_transport_start();
        break;
        case MsgTypeTransportStop:
            // g_state_play = false; // should be this easy, ie sequencer takes care of synthesizer fade outs (when stop is signalled from GUI)
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
    return 1;
}

int muxaudio_mq_to_audio_visit() {
    if (g_msg_to_audio_reader == g_msg_to_audio_writer) {
        return 0;
    }
    struct MuxaudioMsg msg = g_msg_to_audio[g_msg_to_audio_reader];
    muxaudio_mq_to_audio_handle_message(msg);
    msg.type = MsgTypeInit; // mark this as free, FIX: not needed
    g_msg_to_audio_reader = (g_msg_to_audio_reader + 1) % MAILBOX_SIZE;
}

/* this is called from jack */
void muxaudio_msg_from_audio(MuxaudioMsgType typ, int val)
{
    struct MuxaudioMsg msg;
    msg.type = typ;
    msg.payload.i = val;
    muxaudio_mq_from_audio_writer_put(msg);
}

/*
 * Audio ringbuffer from mux to audio
 */
#define MUX_CHAN 2
#define MUX_RINGSIZE (128*MUX_CHAN)
#define MUX_READER_USLEEP 500
#define MUX_WRITER_USLEEP 500

unsigned int g_ringBufferWriterStart = 0;
unsigned int g_ringBufferReaderStart = 0;
struct MuxaudioBuffer g_ringBufferStereo[MUX_RINGSIZE];

unsigned int g_readerCycle = 0;
unsigned int g_writerCycle = 0;
unsigned int g_readerPause = 0;
unsigned int g_writerPause = 0;

bool g_buffer_initial_full = false;

// the muxaudio-buffer number of floats might not
// sum up to the number of floats requested by jack/audio
// there for an counter is needed so we can consume a chunk piecewise
int g_buffer_chunk_pos = 0;

int peek_ringbuffer (unsigned int newReaderPos) {
    int slept = 0;
    for(;;) {
        // ensure we dont read into writers buffer part
        if (// ringbuffer is empty
            g_ringBufferReaderStart == g_ringBufferWriterStart ||
            // if reader wraps around and goes beyond writer
            (g_ringBufferReaderStart > newReaderPos &&
             newReaderPos > g_ringBufferWriterStart) ||
            // no wrap, writer is at right side of reader, but new reader goes beyond writer
            (g_ringBufferWriterStart > g_ringBufferReaderStart &&
             newReaderPos > g_ringBufferWriterStart)) {
            g_readerPause++;
            // we're in realtime-context, and shouldn't do any syscalls,
            // or sleep because there is no maximum sleep-amount guarantees,
            // if paranoid, just spin-loop
            slept++;
            std::this_thread::sleep_for(std::chrono::microseconds(MUX_READER_USLEEP));
        } else { // there is enough room in reader-part of ring-buffer to use
                break;
        }
    }
    return slept;
}

// this function is called by the realtime-context,
// and is reading from the ring-buffer
int mux_process_bufferStereo(unsigned int numFrames, float* bufferStereo){
    LD("mux_process_bufferStereo %i.%i / %i numFrames=%i", g_ringBufferReaderStart, g_buffer_chunk_pos, g_ringBufferWriterStart, numFrames);
    int numFloats = numFrames * MUX_CHAN;
    if (! g_buffer_initial_full) {
        memset(bufferStereo, 0, sizeof(float) * numFloats);
        return -1;
    }

    // if diff is too close to zero, muxseq hasn't feed us enough buffers
    int diff = (g_ringBufferWriterStart - g_ringBufferReaderStart) * MUX_CHUNK_NUMFLOATS;
    if (diff < 2048 && diff > -2048) {
        LD("BUFFER-LOW-WATER-MARK: %i (%i / %i)", diff, g_ringBufferReaderStart, g_ringBufferWriterStart);
    }

    int slept = 0;
    int floatsLeft = numFloats;
    while (floatsLeft) {
        LD("------ floatsLeft: %i", floatsLeft);
        if (g_buffer_chunk_pos == 0) { // no partial slot currently read
            unsigned int newReaderPos = (g_ringBufferReaderStart + 1) % MUX_RINGSIZE;
            slept += peek_ringbuffer(newReaderPos);
            struct MuxaudioBuffer* mabuf = g_ringBufferStereo + newReaderPos;
            // if copy from current slot is partial, update chunk position
            if (floatsLeft < MUX_CHUNK_NUMFLOATS) {
                g_buffer_chunk_pos = floatsLeft;
            }
            for (int i = 0; floatsLeft > 0 && i < MUX_CHUNK_NUMFLOATS; i++) {
                bufferStereo[i] = mabuf->buf[i];
                floatsLeft--;
            }
            if (! g_buffer_chunk_pos) { // only advance if slot is fully consumed
                // advance ringbuffer read position
                if (newReaderPos < g_ringBufferReaderStart) {
                    g_readerCycle++;
                }
                g_ringBufferReaderStart = newReaderPos;
            }
        } else {
            // get current partially slot (note, it isn't advance since that would mark it as read)
            unsigned int newReaderPos = (g_ringBufferReaderStart + 1) % MUX_RINGSIZE;
            struct MuxaudioBuffer* mabuf = g_ringBufferStereo + newReaderPos;
            // copy from current slot, maybe leaving partial read slot
            for (int i = 0; floatsLeft > 0 && i < MUX_CHUNK_NUMFLOATS; i++) {
                bufferStereo[i] = mabuf->buf[g_buffer_chunk_pos + i];
                floatsLeft--;
                g_buffer_chunk_pos++;
            }
            if (g_buffer_chunk_pos == MUX_CHUNK_NUMFLOATS) { // consumed whole slot
                g_buffer_chunk_pos = 0;
            }
            if (! g_buffer_chunk_pos) { // only advance if slot is fully consumed
                // advance ringbuffer read position
                if (newReaderPos < g_ringBufferReaderStart) {
                    g_readerCycle++;
                }
                g_ringBufferReaderStart = newReaderPos;
            }
        }
    }
    if (slept > 0) {
        LW("WARNING: jack had to wait %i * %i usecs", slept, MUX_READER_USLEEP);
    }
    return slept;
}

int muxaudio_audio_process_work() {
    unsigned int newWriterPos = (g_ringBufferWriterStart + 1) % MUX_RINGSIZE;
    // ensure we dont overwrite part of buffer that is being read by reader
    // if writer wraps around and goes beyond reader
    if (g_ringBufferReaderStart < g_ringBufferWriterStart &&
        g_ringBufferWriterStart > newWriterPos &&
        newWriterPos >= g_ringBufferReaderStart) {
        if(g_state_play) g_buffer_initial_full = true;
        return 0;
    // wraps around, but goes beyond reader
    } else if (g_ringBufferWriterStart < g_ringBufferReaderStart &&
               newWriterPos < g_ringBufferWriterStart) {
        if(g_state_play) g_buffer_initial_full = true;
        return 0;
    // no wrap, reader is at right side of writer, but new writer goes beyond reader
    } else if (g_ringBufferWriterStart < g_ringBufferReaderStart &&
               newWriterPos >= g_ringBufferReaderStart) {
        if(g_state_play) g_buffer_initial_full = true;
        return 0;
    }

    // Request a chunk over the network from mux/seq
    struct MuxaudioBuffer* mabuf = g_ringBufferStereo + newWriterPos;
    struct MuxaudioMsg msg;
    msg.type = MsgTypeAudioBufferFeed;
    if (zmq_send(g_socket_audio.socket, &msg, sizeof(struct MuxaudioMsg), 0) < 0) return -1;
    if (zmq_recv(g_socket_audio.socket, mabuf, sizeof(struct MuxaudioBuffer), 0) < 0) return -1;
    if (newWriterPos < g_ringBufferWriterStart) {
        g_writerCycle++;
    }
    g_ringBufferWriterStart = newWriterPos;
    return 1;
}

// this is the non-realtime part, and is requested to do work
// by the realtime-part, and then buffering its work-content
// and is writing to the ring-buffer
void muxaudio_audio_process() {
    g_mux_audio_process_run = 1;
    while (g_mux_audio_process_run) {
        bool workDone = false;
        int rc = muxaudio_audio_process_work();
        if (rc >= 0) { // error, sleep longer
            workDone = true;
        }
        if (muxaudio_mq_from_audio_reader_visit()) { // and message-queue is empty
            workDone = true;
        }
        if (! workDone) {
            g_writerPause++;
            std::this_thread::sleep_for(std::chrono::milliseconds(MUX_WRITER_USLEEP));
        }
    }
    LD("MUX audio-process terminated.");
}

void muxaudio_audio_process_stop () {
    g_mux_audio_process_run = 0;
}

void muxaudio_network_mainloop_ctrl()
{
    LD("MUXAUDIO ZeroMQ control entering main-loop");
    while (1) {
        struct MuxaudioMsg msg;
        if (zmq_recv(g_socket_ctrl.socket, &msg, sizeof(struct MuxaudioMsg), 0) < 0) {
            LE("zmq-recv control error: %s", strerror(errno));
            break;
        }
        LD("Received Control Message, type=(%i)%s", msg.type, muxaudio_msg_type_info(msg.type));
        mux_mq_to_audio_writer_put(msg);
        if (zmq_send(g_socket_ctrl.socket, &msg, sizeof(struct MuxaudioMsg), 0) < 0) {
            LE("zmq-send control error: %s", strerror(errno));
            break;
        }
    }
    LD("mux_network_mainloop control has exited");
}

/*
void mux_teardown_driver (JackAudio *driver) {
    if (driver) {
        stopWait();
        delete driver;
        driver = 0;
    }
}
*/

/* This thread is setting up JACK callback (which inturn will read from audio-ringbuffer).
 * Then check any incoming control-messages from ZMQ
 */
void mux_network_mainloop_audio()
{
    LD("MUX ZeroMQ audio entering main-loop");
    g_driver = driverFactory("jack");
    while (1) {
        if (! muxaudio_mq_to_audio_visit()) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
        }
    }
    LD("mux_network_mainloop audio has exited");
}

void muxaudio_network_server_audio()
{
    Mux::mux_make_connection(g_socket_audio, "tcp://*:7712", Mux::ZmqType::QUERY, Mux::ZmqDir::REQ, Mux::ZmqServer::BIND);
    //FIX: this should be called by the thread that calls mux_audio_process() {
    mux_network_mainloop_audio();
}

void muxaudio_network_server_ctrl()
{
    Mux::mux_make_connection(g_socket_ctrl, "tcp://*:7711", Mux::ZmqType::QUERY, Mux::ZmqDir::REP, Mux::ZmqServer::BIND);
    muxaudio_network_mainloop_ctrl();
}

} // end of namespace MS

/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 */
/*
 *
 *
 */

#include "event.h"
#include "mux.h"
#include "muxcommon.h"
#include "muxlib.h"

namespace Ms {

struct Mux::MuxSocket g_muxseq_query_client_socket;
struct Mux::MuxSocket g_muxseq_queryreq_client_socket;

static int muxseq_query_zmq (MuxseqMsgType type, MuxseqMsg &msg) {
    //LD("muxseq_query_zmq %s + msg", muxseq_msg_type_info(type));
    //LD("MSCORE ==> MUXSEQ query msg %s", muxseq_msg_type_info(type));
    msg.type = type;
    Mux::mux_zmq_send(g_muxseq_query_client_socket, (void*) &msg, sizeof(struct MuxseqMsg));
    return Mux::mux_zmq_recv(g_muxseq_query_client_socket, (void*) &msg, sizeof(struct MuxseqMsg));
}

int muxseq_send (MuxseqMsgType type) {
    LD("muxseq_send %s", muxseq_msg_type_info(type));
    struct MuxseqMsg msg;
    return muxseq_query_zmq(type, msg);
}

int muxseq_send (MuxseqMsgType type, int i) {
    LD("muxseq_send %s int=%i", muxseq_msg_type_info(type), i);
    struct MuxseqMsg msg;
    msg.payload.i = i;
    //qDebug("muxseq-send msg %s, int=%i", muxseq_msg_type_info(type), msg.payload.i);
    return muxseq_query_zmq(type, msg);
}

int muxseq_send (MuxseqMsgType type, double d) {
    LD("muxseq_send %s float=%f", muxseq_msg_type_info(type), d);
    struct MuxseqMsg msg;
    msg.payload.d = d;
    //qDebug("muxseq-send msg %s, double=%f", muxseq_msg_type_info(type), d);
    return muxseq_query_zmq(type, msg);
}

int muxseq_send (MuxseqMsgType type, NPlayEvent event) {
    LD("muxseq_send %s NPlayEvent", muxseq_msg_type_info(type));
    struct MuxseqMsg msg;
    muxseq_msg_set_NPlayEvent(msg, event);
    return muxseq_query_zmq(type, msg);
}

int muxseq_send (MuxseqMsgType type, int maxMidiPorts, std::vector<struct SparseMidiEvent> sevs) {
    int slen = sevs.size();
    int len = 12 + sizeof(struct SparseMidiEvent) * sevs.size() + sizeof(struct MuxseqMsg); // need atleast large enough to hold an generic struct MuxseqMsg
    unsigned char* buf = (unsigned char*) malloc(len);
    memset(buf, 0, len);
    memcpy(buf,    &type, 4);
    memcpy(buf+4,  &maxMidiPorts, 4);
    memcpy(buf+8,  &slen, 4);
    struct SparseMidiEvent *sevs2 = (struct SparseMidiEvent *) (buf + 12);
    memcpy(sevs2, &sevs[0], sizeof(struct SparseMidiEvent) * slen);
    LD("muxseq_send %s maxMidiPorts+sevs (%d bytes)", muxseq_msg_type_info(type), len);
    int rc = Mux::mux_zmq_send(g_muxseq_query_client_socket, buf, len);
    free(buf);
    if (rc != len) {
        return -1;
    }
    struct MuxseqMsg msg;
    msg.type = (MuxseqMsgType) type;
    return Mux::mux_zmq_recv(g_muxseq_query_client_socket, (void*) &msg, sizeof(struct MuxseqMsg));
}
       

int muxseq_query (MuxseqMsgType type) {
    LD("muxseq_query %s", muxseq_msg_type_info(type));
    struct MuxseqMsg msg;
    muxseq_query_zmq(type, msg);
    return 0;
}

bool muxseq_query_bool (MuxseqMsgType type) {
    LD("muxseq_query_bool %s", muxseq_msg_type_info(type));
    struct MuxseqMsg msg;
    muxseq_query_zmq(type, msg);
    LD("muxseq_query_bool %s => %i", muxseq_msg_type_info(type), msg.payload.b);
    return msg.payload.b;
}

double muxseq_query_float (MuxseqMsgType type) {
    LD("muxseq_query_float %s", muxseq_msg_type_info(type));
    struct MuxseqMsg msg;
    muxseq_query_zmq(type, msg);
    LD("muxseq_query_float %s => %f", muxseq_msg_type_info(type), msg.payload.d);
    return msg.payload.d;
}

void muxseq_query (MuxseqMsgType type, bool b) {
    LD("muxseq_query %s bool %i", muxseq_msg_type_info(type), b);
    struct MuxseqMsg msg;
    msg.payload.b = b;
    muxseq_query_zmq(type, msg);
    return;
}

int mux_query_recv_Muxseq (Mux::MuxSocket &sock, struct MuxseqMsg &msg)
{
    int rlen;
    void* m = mux_query_recv(sock, &rlen);
    if (! m) return -1;
    if (rlen != sizeof(struct MuxseqMsg)) return -1;
    memcpy(&msg, m, sizeof(struct MuxseqMsg));
    free(m);
    return 0;
}

void muxseq_create(int sampleRate) { // called from musescore.cpp: MuseScore::init
    muxseq_send(MsgTypeSeqCreate, sampleRate);
}

void muxseq_dealloc() {
    muxseq_send(MsgTypeSeqDeinit);
}

void muxseq_exit() {
    muxseq_send(MsgTypeSeqExit);
}


bool muxseq_seq_init (bool hotPlug) {
    muxseq_query(MsgTypeSeqInit, hotPlug);
    return true; //FIX use the return value (when implemented in muxseq)
}

void muxseq_seq_start () {
    muxseq_send(MsgTypeSeqStart);
}

bool muxseq_seq_alive() {
    //FIX: perhaps locally cache this?
    muxseq_query(MsgTypeSeqAlive);
    return true;
}

void muxseq_stop_notes () {
    muxseq_send(MsgTypeSeqStopNotes);
}

void muxseq_stop_notes (int channel) {
    muxseq_send(MsgTypeSeqStopNotes, channel);
}

void muxseq_stop_notetimer () {
    muxseq_send(MsgTypeSeqStopNoteTimer);
}

void muxseq_start_notetimer (int duration) {
    muxseq_send(MsgTypeSeqStartNoteTimer);
}

void muxseq_stop_wait () {
    muxseq_send(MsgTypeSeqStopWait);
}

bool muxseq_seq_playing() {
    return muxseq_query_bool(MsgTypeSeqPlaying);
}

bool muxseq_seq_running() {
    return muxseq_query_bool(MsgTypeSeqRunning);
}

bool muxseq_seq_stopped() {
    return muxseq_query_bool(MsgTypeSeqStopped);
}

bool muxseq_seq_can_start() {
    return muxseq_query_bool(MsgTypeSeqCanStart);
}

int muxseq_seq_curTick() {
    return muxseq_query_float(MsgTypeSeqCurTick);
}

float muxseq_seq_curTempo() {
    return muxseq_query_float(MsgTypeSeqCurTempo);
}

void muxseq_seq_setRelTempo (double tempo) {
    muxseq_send(MsgTypeSeqSetRelTempo, tempo);
}

#define DEFUNMUXSEQVOID(name, sname) \
  void muxseq_seq_ ## name() { \
      muxseq_send(MsgType ## sname); \
  }

DEFUNMUXSEQVOID(nextMeasure, NextMeasure)
DEFUNMUXSEQVOID(nextChord,   NextChord)
DEFUNMUXSEQVOID(prevMeasure, PrevMeasure)
DEFUNMUXSEQVOID(prevChord,   PrevChord)
DEFUNMUXSEQVOID(rewindStart, RewindStart)
DEFUNMUXSEQVOID(seekEnd,     SeekEnd)
DEFUNMUXSEQVOID(setLoopIn,   SetLoopIn);
DEFUNMUXSEQVOID(setLoopOut,  SetLoopOut);
DEFUNMUXSEQVOID(setLoopSelection, SetLoopSelection);
DEFUNMUXSEQVOID(recomputeMaxMidiOutPort, RecomputeMaxMidiOutPort);

float muxseq_seq_metronomeGain() {
    //FIX: return seq3->metronomeGain();
    return 1.0f;
}

void muxseq_seq_playMetronomeBeat(BeatType beatType) {
    //FIX: muxseq_send(MsgTypeSeqplayMetronomeBeat(beatType));
}

void muxseq_seq_stop () {
    muxseq_send(MsgTypeSeqStop);
}

void muxseq_send_event(NPlayEvent event) {
    muxseq_send(MsgTypeSeqSendEvent, event);
}

void muxseq_seq_initInstruments() {
    //FIX: seq3->initInstruments();
}

void muxseq_preferencesChanged() {
    muxseq_send(MsgTypeSeqPreferencesChanged);
}


}

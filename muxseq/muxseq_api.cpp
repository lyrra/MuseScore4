/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 */
/*
 *
 *
 */

#include "mux.h"
#include "muxseqsig.h"
#include "event.h"
#include "muxlib.h"
#include "seq.h"
#include "scoreview.h"
#include "msynthesizer.h"
#include "effects/zita1/zita.h"
#include "effects/compressor/compressor.h"
#include "effects/noeffect/noeffect.h"
#include "fluid/fluid.h"
#include "synthesizer.h"
//#include "synthesizergui.h"
#include "muxseq.h"

#ifdef AEOLUS
extern Ms::Synthesizer* createAeolus();
#endif

#ifdef ZERBERUS
extern Ms::Synthesizer* createZerberus();
#endif

namespace Ms {
extern struct Mux::MuxSocket g_muxsocket_mscoreQueryReqServer;
extern Seq* g_seq;
MasterSynthesizer* synti = 0;

void _logstr (char *str) {
    qDebug(str);
}

int  muxseq_send(MuxseqMsgType type) {
    LD("muxseq msg %i (NOT IMPL)", type);
    return 0;
}

int  muxseq_send(MuxseqMsgType type, int i) {
    LD("muxseq msg %i about int %i (NOT IMPL)", type, i);
    return 0;
}
int  muxseq_send(MuxseqMsgType type, double d) {
    LD("muxseq msg %i about int %f (NOT IMPL)", type, d);
    return 0;
}

int  muxseq_send(MuxseqMsgType type, NPlayEvent event) {
    LD("muxseq msg %i about event (NOT IMPL)", type);
    return 0;
}

int  muxseq_query(MuxseqMsgType type) {
    LD("muxseq msg query %i (NOT IMPL)", type);
    return 0;
}

bool muxseq_query_bool(MuxseqMsgType type) {
    LD("muxseq msg query %i (NOT IMPL)", type);
    return true;
}

double muxseq_query_float(MuxseqMsgType type) {
    LD("muxseq msg query %i (NOT IMPL)", type);
    return 0.0;
}

void muxseq_query(MuxseqMsgType type, bool b) {
    LD("muxseq msg query %i about bool %i (NOT IMPL)", type, b);
}

int muxseq_mscore_send (MuxseqMsgType type, int i) {
    /* Create a new message, allocating 6 bytes for message content */
    struct MuxseqMsg msg;
    msg.type = type;
    msg.payload.i = i;
    strcpy(msg.label, "muxseq");
    return mux_query_send(g_muxsocket_mscoreQueryReqServer, &msg, sizeof(struct MuxseqMsg));
}

void* muxseq_mscore_recv (MuxseqMsgType type) {
    int rlen2;
    void *m = mux_query_recv(g_muxsocket_mscoreQueryReqServer, &rlen2);
    LD8("muxseq => mscore got reply, bufSize=%i", rlen2);
    if (! m) {
        LE("muxseq_mscore_query recv failed (expected reply to msg %s)", muxseq_msg_type_info(type));
        return nullptr;
    }
    return m;
}

//FIX: handles only payload  'i'
void* muxseq_mscore_query (MuxseqMsgType type, int i)
{
    LD6("MUXSEQ ==> MSCORE query msg %s", muxseq_msg_type_info(type));
    if (muxseq_mscore_send (type, i) < 0) {
        LE("muxseq_mscore_query send failed");
        return nullptr;
    }
    // receive reply, which is a 'struct MuxseqEventsHeader' followed by an list of events
    void *m = muxseq_mscore_recv(type);
    struct MuxseqEventsHeader *meh;
    meh = (struct MuxseqEventsHeader *) m;
    meh->sevs = (struct SparseEvent *) (m + sizeof(struct MuxseqEventsHeader));
    /* Release message */
    return meh;
}

//FIX: should put on ringbuffer, which in turn uses zmq-network
int muxseq_mscore_tell (MuxseqMsgType type, int i)
{
    LD6("MUXSEQ ==> MSCORE tell msg %s", muxseq_msg_type_info(type));
    if (muxseq_mscore_send (type, i) < 0) {
        LE("muxseq_mscore_query send failed");
        return -1;
    }
    // receive reply, which is a 'struct MuxseqEventsHeader' followed by an list of events
    void *m = muxseq_mscore_recv(type);
    free(m);
    return 0;
}

#define DEFMUXSEQVOID(name, sname) \
  void muxseq_seq_ ## name() { \
      muxseq_send(MsgType ## sname); \
  }

MasterSynthesizer* muxseq_create_synti(int sampleRate);

void muxseq_initialize(int sampleRate) { // called from musescore.cpp: MuseScore::init
    muxseq_send(MsgTypeSeqInit, sampleRate);
}

void muxseq_dealloc() {
    muxseq_send(MsgTypeSeqDeinit);
}

void muxseq_exit() {
    muxseq_send(MsgTypeSeqExit);
}

bool muxseq_seq_alive() {
    //FIX: perhaps locally cache this?
    muxseq_query(MsgTypeSeqAlive);
    return true;
}

bool muxseq_seq_init (bool hotPlug) {
    muxseq_query(MsgTypeSeqInit, hotPlug);
    return true;
}

void muxseq_seq_start () {
    LD("muxseq_seq_start");
    muxseq_send(MsgTypeSeqStart);
}

void muxseq_seq_stop () {
    muxseq_send(MsgTypeSeqStop);
}

void muxseq_send_event(NPlayEvent event) {
    muxseq_send(MsgTypeSeqSendEvent, event);
}

void muxseq_start_note(int channel, int pitch, int velocity, double nt) {
    muxseq_send(MsgTypeSeqStartNote);
}

void muxseq_start_note_dur(int channel, int pitch, int velocity, int duration, double nt) {
    muxseq_send(MsgTypeSeqStartNoteDur);
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

int muxseq_seq_seek(int ticks) {
    return muxseq_send(MsgTypeSeqSeek, ticks);
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

DEFMUXSEQVOID(nextMeasure, NextMeasure)
DEFMUXSEQVOID(nextChord,   NextChord)
DEFMUXSEQVOID(prevMeasure, PrevMeasure)
DEFMUXSEQVOID(prevChord,   PrevChord)
DEFMUXSEQVOID(rewindStart, RewindStart)
DEFMUXSEQVOID(seekEnd,     SeekEnd)
DEFMUXSEQVOID(setLoopIn,   SetLoopIn);
DEFMUXSEQVOID(setLoopOut,  SetLoopOut);
DEFMUXSEQVOID(setLoopSelection, SetLoopSelection);
DEFMUXSEQVOID(recomputeMaxMidiOutPort, RecomputeMaxMidiOutPort);

float muxseq_seq_metronomeGain() {
    //FIX: return seq3->metronomeGain();
    return 1.0f;
}

void muxseq_seq_playMetronomeBeat(BeatType beatType) {
    //FIX: muxseq_send(MsgTypeSeqplayMetronomeBeat(beatType));
}

void muxseq_seq_initInstruments() {
    //FIX: seq3->initInstruments();
}

void muxseq_preferencesChanged() {
    muxseq_send(MsgTypeSeqPreferencesChanged);
}

void* muxseq_seq_score () {
    //FIX: return seq3->score();
    return nullptr;
}

void muxseq_seq_set_scoreview (void* v) {
    //FIX: seq shouldn't know about the score, but instead take an eventmap
    //FIX: seq3->setScoreView((ScoreView*)v);
}

void muxseq_seq_setController(int channel, int vol, int iv) {
    //FIX: seq3->setController(channel, vol, iv);
}

void muxseq_seq_updateOutPortCount(int maxPorts) {
    muxseq_send(MsgTypeSeqUpdateOutPortCount, maxPorts);
}

// signals

#if 0
MuxSeqSig* muxseq_init_muxseqsig() {
    return muxseqsig_init();
}

void muxseq_seq_emit_started () {
    muxseqsig_seq_emit_started();
}

void muxseq_seq_emit_stopped () {
    muxseqsig_seq_emit_stopped();
}
#endif

// synthesizer
MasterSynthesizer* muxseq_synthesizerFactory() {
    MasterSynthesizer* ms = new MasterSynthesizer();

    FluidS::Fluid* fluid = new FluidS::Fluid();
    ms->registerSynthesizer(fluid);

#ifdef AEOLUS
    ms->registerSynthesizer(::createAeolus());
#endif
#ifdef ZERBERUS
    ms->registerSynthesizer(createZerberus());
#endif
    ms->registerEffect(0, new NoEffect);

#ifdef ZITA_REVERB
    ms->registerEffect(0, new ZitaReverb);
#endif

    ms->registerEffect(0, new Compressor);
    // ms->registerEffect(0, new Freeverb);
    ms->registerEffect(1, new NoEffect);

#ifdef ZITA_REVERB
    ms->registerEffect(1, new ZitaReverb);
#endif

    ms->registerEffect(1, new Compressor);
    // ms->registerEffect(1, new Freeverb);
    ms->setEffect(0, 1);
    ms->setEffect(1, 0);
    return ms;
}

MasterSynthesizer* muxseq_create_synti(int sampleRate) {
    LD("muxseq_create_synti initialize synthesizers");
    synti = muxseq_synthesizerFactory();
    synti->setSampleRate(sampleRate);
    synti->init();
    if (g_seq == nullptr) {
        LW("muxseq_create_synti cant initialize synthesizers: sequencer is not initialized!");
        return nullptr;
    }
    g_seq->setMasterSynthesizer(synti);
    return synti;
}

MasterSynthesizer* muxseq_get_synti() {
    return synti;
}

void muxseq_delete_synti() {
    delete synti;
    synti = nullptr;
}

bool muxseq_synti () {
    //FIX: return seq3->synti();
    return true;
}

void muxseq_synti_init() {
    //FIX: seq3->synti()->init();
}

SynthesizerState muxseq_synti_get_synthesizerState() {
    //MasterSynthesizer* synti = muxseq_get_synti();
    SynthesizerState state;
    //FIX: shouln't need to check if synti is created
    //FIX: return synti ? synti->state() : state;
    return state;
}

void muxseq_synti_setState(SynthesizerState state) {
  //FIX: synti->setState(state());
}
        
void muxseq_synti_storeState() {
  // synti->storeState();
}


float muxseq_synti_getGain () {
    //FIX: return synti->gain();
    return 1.0f;
}

void muxseq_synti_setGain (double gain) {
  //FIX: serverside  muxseq_get_synti()->setGain(gain);
}

float muxseq_synti_getMinGainAsDecibels () {
  //FIX: serverside synti->minGainAsDecibels;
  return 0.0f;
}
float muxseq_synti_getMaxGainAsDecibels () {
  //FIX: synti->maxGainAsDecibels;
  return 0.0f;
}
float muxseq_synti_getGainAsDecibels () {
  //FIX:  synti->gainAsDecibels();
  return 0.0f;
}

float muxseq_synti_getDefaultGainAsDecibels () {
  //FIX: synti->defaultGainAsDecibels;
  return 0.0f;
}

void muxseq_synti_setGainAsDecibels(float gain) {
  //FIX
}

void muxseq_synti_setMasterTuning(float tuning) {
  //FIX
  // synti->setMasterTuning(tuning);
}

float muxseq_synti_getMasterTuning() {
  // FIX: synti->masterTuning();
  return 0.0f;
}

void muxseq_synti_setSampleRate (float sampleRate) {
    //FIX: seq3->synti()->setSampleRate(sampleRate);
}

void muxseq_synti_setEffect(int effectIdx, int n) {
  // FIX synti->setEffect(effectIdx, n);
}

int muxseq_synti_getDynamicsMethod() {
  // synti->dynamicsMethod();
  return 0;
}
void muxseq_synti_setDynamicsMethod(int n) {
  // FIX synti->setDynamicsMethod(val);
}

int muxseq_synti_getCcToUseIndex() {
  //FIX: synti->ccToUseIndex();
        return 0;
}

int muxseq_synti_getIndexOfEffect(int n) {
  //FIX: synti->indexOfEffect(n);
  return 0;
}

void muxseq_synti_setCcToUseIndex(int n) {
  //FIX
}

QList<MidiPatch*> muxseq_synti_getPatchInfo () {
  QList<MidiPatch*> pl;
  // FIX: populate list
  return pl;
}


MasterSynthesizer* muxseq_synth_create (int sampleRate, SynthesizerState synthState) {
    MasterSynthesizer* synth = muxseq_synthesizerFactory();
    synth->init();
    synth->setSampleRate(sampleRate);
    bool r = synth->setState(synthState);
    if (!r || !synth->hasSoundFontsLoaded()) {
        synth->init();
    }
    return synth;
}

void muxseq_synth_delete (MasterSynthesizer* synth) {
    delete synth;
}

Synthesizer* muxseq_synth_get_name(const QString& name) {
    return synti ? synti->synthesizer(name) : nullptr;
}

void muxseq_synth_load_soundfonts (Synthesizer* s, QStringList sfList) {
    for (auto sf : sfList) {
        s->addSoundFont(sf);
    }
    if (!sfList.isEmpty()) {
        synti->storeState();
    }
    //FIX: send to muxseq-client s->gui()->synthesizerChanged();
}

void muxseq_synth_fluid_load_soundfonts (QStringList sfList) {
    Synthesizer* s = synti->synthesizer("Fluid");
    muxseq_synth_load_soundfonts(s, sfList);
}

void muxseq_synth_zerberus_load_soundfonts (QStringList sfzList) {
    Synthesizer* s = synti->synthesizer("Zerberus");
    muxseq_synth_load_soundfonts(s, sfzList);
}

void muxseq_synth_unload_soundfonts (Synthesizer* s, QStringList sfList) {
    for (auto sf : sfList) {
        s->removeSoundFont(sf);
    }
    if (!sfList.isEmpty()) {
        synti->storeState();
    }
    //FIX: send to muxseq-client: s->gui()->synthesizerChanged();
}

void muxseq_synth_fluid_unload_soundfonts (QStringList sfList) {
    Synthesizer* s = synti->synthesizer("Fluid");
    muxseq_synth_unload_soundfonts(s, sfList);
}

void muxseq_synth_zerberus_unload_soundfonts (QStringList sfzList) {
    Synthesizer* s = synti->synthesizer("Zerberus");
    muxseq_synth_unload_soundfonts(s, sfzList);
}

} // namespace Ms

/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 */
/*
 *
 *
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "event.h"
#include "libmscore/repeatlist.h"
#include "libmscore/synthesizerstate.h"
#include "libmscore/rendermidi.h"
#include "libmscore/tempo.h"
#include "synthesizer.h"
#include "mux.h"
#include "muxcommon.h"
#include "muxlib.h"
#include "muxseqsig.h"
#include "scoreview.h"
#include "msynthesizer.h"
#include "muxseq_client.h"
#include "muxseq_net.h"
#include "musescore.h" //FIX: too much to include, needed for extern *mscore

#include "libmscore/rendermidi.h"
#include "libmscore/score.h"
#include "muxtools/eventutils.h"

#define LEX(...) qFatal(__VA_ARGS__)

namespace Ms {


static constexpr int minUtickBufferSize = 480 * 4 * 10; // about 10 measures of 4/4 time signature

/* initialization/control
 */
extern struct Mux::MuxSocket g_muxseq_query_client_socket;
extern struct Mux::MuxSocket g_muxseq_queryreq_client_socket;
static bool g_threads_started = false;
static bool g_thread_musescoreQuery_started = false;
static bool g_thread_musescoreBulletin_started = false;
static std::vector<std::thread> muxseq_Threads;
/**/
// these are an mscore-side Seq object
static ScoreView* g_cv = nullptr;
static MasterScore* g_cs = nullptr;
static MidiRenderer g_midi(nullptr);
extern MuseScore *mscore;


#define L_MUX_QUERY(type) \
  qDebug("muxseq_client query %s", muxseq_msg_type_info(type));


void maybe_update_midiRenderer()
{
      Score* cs = g_cv ? g_cv->score()->masterScore() : 0;
      g_midi = MidiRenderer(cs);
      g_midi.setMinChunkSize(10);
}

void _renderChunk(RangeMap &renderEventsStatus,
                  const MidiRenderer::Chunk& ch,
                  EventMap* eventMap)
{
    SynthesizerState synState = mscore->synthesizerState();
    MidiRenderer::Context ctx(synState);
    ctx.metronome = true;
    ctx.renderHarmony = true;
    g_midi.renderChunk(ch, eventMap, ctx);
    renderEventsStatus.setOccupied(ch.utick1(), ch.utick2());
}

//FIX: put this is muxlib
unsigned char* eventMap_to_muxbuffer(MuxseqMsgType type, EventMap evm,
                                     qreal beatsPerSecond, int *rlen) {
    size_t numEvents = evm.size();
    struct MuxseqEventsHeader head;
    head.numEvents = numEvents;
    head.type = type;
    int len = sizeof(struct MuxseqEventsHeader) +
              sizeof(struct SparseEvent) * numEvents;
    unsigned char* buf = (unsigned char*) malloc(len);
    memcpy(buf, &head, sizeof(struct MuxseqEventsHeader));
    struct SparseEvent *sevs = (struct SparseEvent *) (buf + sizeof(struct MuxseqEventsHeader));
    size_t i = 0;
    for(const auto &pair: evm) {
        int framepos = pair.first;
        NPlayEvent nev = pair.second;
        sevs[i].framepos = framepos;
        sevs[i].type = nev.type();
        sevs[i].channel = nev.channel();
        sevs[i].pitch = nev.pitch();
        sevs[i].velo  = nev.velo();
        // derived
        sevs[i].midiPort = g_cs->midiPort(nev.channel());
        sevs[i].playPosSeconds = g_cs->utick2utime(framepos);
        sevs[i].beatsPerSecond = (int) beatsPerSecond;
        sevs[i].division       = (int) MScore::division;
        strcpy(sevs[i].synthName, qPrintable(g_cs->midiMapping(nev.channel())->articulation()->synti()));
        LD4("%i: storing event framepos=%i pitch=%i/%i channel=%i/%i playPosSeconds=%f",
            i, sevs[i].framepos,
            sevs[i].pitch, nev.pitch(),
            sevs[i].channel, nev.channel(),
            sevs[i].playPosSeconds);
        i++;
    }
    LD4("Done computing the payload, numEvents=%i bufLen=%i", numEvents, len);
    *rlen = len;
    return buf;
}

int handle_mscore_msg_SeqStarted (Mux::MuxSocket &sock, struct MuxseqMsg msg)
{
    LD("handle MsgTypeSeqStarted");
    mscore->seqStarted();
    strcpy(msg.label, "mscore");
    if (mux_query_send(sock, &msg, sizeof(struct MuxseqMsg)) == -1) {
        return -1;
    }
    return 0;
}

int handle_mscore_msg_SeqStopped (Mux::MuxSocket &sock, struct MuxseqMsg msg)
{
    int playFrame = msg.payload.i;
    int tck = g_cs->repeatList().utick2tick(g_cs->utime2utick(qreal(playFrame) / qreal(MScore::sampleRate)));
    g_cs->setPlayPos(Fraction::fromTicks(tck));
    g_cs->update();
    mscore->seqStopped();
    if (g_cv) {
        g_cv->setCursorOn(false);
    }
    strcpy(msg.label, "mscore");
    if (mux_query_send(sock, &msg, sizeof(struct MuxseqMsg)) == -1) {
        return -1;
    }
    return 0;
}

int handle_mscore_msg_SeqUTick (Mux::MuxSocket &sock, struct MuxseqMsg msg)
{
    int utick = msg.payload.i;
    int t = 0;
    if (g_cs) {
      t = g_cs->repeatList().utick2tick(utick);
      mscore->currentScoreView()->moveCursor(Fraction::fromTicks(t));
      mscore->setPos(Fraction::fromTicks(t));
    }
    strcpy(msg.label, "mscore");
    msg.payload.i = t;
    if (mux_query_send(sock, &msg, sizeof(struct MuxseqMsg)) == -1) {
        return -1;
    }
    return 0;
}

int handle_mscore_msg_SeqRenderEvents(Mux::MuxSocket &sock, struct MuxseqMsg msg)
{
    maybe_update_midiRenderer();
    int utick = msg.payload.i;
    RangeMap renderEventsStatus;
    renderEventsStatus.clear();
    int unrenderedUtick = renderEventsStatus.occupiedRangeEnd(utick);
    EventMap events;
    LD4("SeqRenderEvents -- utick=%i unrenderedUtick=%i (num events before render: %i)\n", utick, unrenderedUtick, events.size());
    while (unrenderedUtick - utick < minUtickBufferSize) {
        const MidiRenderer::Chunk chunk = g_midi.getChunkAt(unrenderedUtick);
        if (!chunk)
              break;
        _renderChunk(renderEventsStatus, chunk, &events);
        unrenderedUtick = renderEventsStatus.occupiedRangeEnd(utick);
    }
    int rlen;
    unsigned int playPosUTick;
    unsigned char* buf = eventMap_to_muxbuffer(MsgTypeSeqRenderEvents, events,
                                               g_cs->tempomap()->relTempo(), // relTempo needed here to ensure that bps changes as we slide the tempo bar
                                               &rlen);
    // send events to muxseq
    if (mux_query_send(sock, buf, rlen) == -1) {
        free(buf);
        return -1;
    }
    free(buf);
    return 0;
}

int muxseq_network_mainloop_queryrep_recv(Mux::MuxSocket &sock) {
    struct MuxseqMsg msg;
    if (mux_query_recv_Muxseq(sock, msg) < 0) {
        return -1;
    }
    LD6("MSCORE ==> MUXSEQ got query from muxseq: %s label=%s", muxseq_msg_type_info(msg.type), msg.label);
    switch (msg.type) {
        case MsgTypeSeqRenderEvents:
            return handle_mscore_msg_SeqRenderEvents(sock, msg);
        case MsgTypeSeqStarted:
            return handle_mscore_msg_SeqStarted(sock, msg);
        case MsgTypeSeqStopped:
            return handle_mscore_msg_SeqStopped(sock, msg);
        case MsgTypeSeqUTick:
            return handle_mscore_msg_SeqUTick(sock, msg);
        default:
            strcpy(msg.label, "mscore");
            if (mux_query_send(sock, &msg, sizeof(struct MuxseqMsg)) == -1) {
                return -1;
            }
            return 0;
    }
}

void muxseq_network_mainloop_queryrep(Mux::MuxSocket &sock)
{
    while (1) {
        int rc = muxseq_network_mainloop_queryrep_recv(sock);
        if (rc < 0) {
            qFatal("  -- musescore-muxseq queryreq client error");
            return;
        }
    }
}


// void mux_network_close(struct MuxSocket &sock)

void muxseq_query_req_thread_init(std::string _notused)
{
    _set_threadname(">seq ");
    qDebug("Connecting to muxseq query-req-client.");
    Mux::mux_make_connection(g_muxseq_query_client_socket, MUX_MUSESCORE_QUERY_CLIENT_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REQ, Mux::ZmqServer::CONNECT);
    qDebug("Connected to muxseq (as query-req-client).");
    g_thread_musescoreQuery_started = true;
    //muxseq_network_mainloop_query();
}

void muxseq_query_rep_thread_init(std::string _notused)
{
    _set_threadname("<seq ");
    Mux::mux_make_connection(g_muxseq_queryreq_client_socket, MUX_MUSESCORE_QUERYREQ_CLIENT_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REP, Mux::ZmqServer::CONNECT);
    //g_thread_musescoreBulletin_started = true;
    muxseq_network_mainloop_queryrep(g_muxseq_queryreq_client_socket);
}

void mux_musescore_client_start()
{
    if (g_threads_started) {
        qWarning("musescore-mux-client already started");
        return;
    }
    std::vector<std::thread> threadv;
    //
    std::thread zmqMuxseqQueryThread(muxseq_query_req_thread_init, "notused");
    threadv.push_back(std::move(zmqMuxseqQueryThread));
    //
    std::thread zmqMuxseqBulletinThread(muxseq_query_rep_thread_init, "notused");
    threadv.push_back(std::move(zmqMuxseqBulletinThread));
    // move threads to heap
    muxseq_Threads = std::move(threadv);
    while(g_thread_musescoreQuery_started == false /* ||
          g_thread_musescoreBulletin_started == false */) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    g_threads_started = true;
}

/* Musescore/client specific
 *
 */

MasterSynthesizer* synti = 0;

void muxseq_start_note(int channel, int pitch, int velocity, double nt) {
    muxseq_send(MsgTypeSeqStartNote);
}

void muxseq_start_note_dur(int channel, int pitch, int velocity, int duration, double nt) {
    //FIX: create an sparse midi event
    muxseq_send(MsgTypeSeqStartNoteDur);
}

void muxseq_seq_seek(int utick) {
    LD4("muxseq_seq_seek utick=%i", utick);
    muxseq_send(MsgTypeSeqSeek, utick);
    if (g_cs) {
        int t = g_cs->repeatList().utick2tick(utick);
        Segment* seg = g_cs->tick2segment(Fraction::fromTicks(t));
        if (seg)
            mscore->currentScoreView()->moveCursor(seg->tick());
        g_cs->setPlayPos(Fraction::fromTicks(t));
        g_cs->update();
    }
}

static void add_SparseMidiEvent (std::vector<SparseMidiEvent> &sevs, int type, int channel, int dataA, int dataB, const char *synthName) {
    struct SparseMidiEvent sev;
    sev.type = type;
    sev.channel = channel;
    sev.dataA = dataA;
    sev.dataB = dataB;
    strcpy(sev.synthName, synthName);
    sev.framepos = 0;
    sev.midiPort = 0;
    sevs.push_back(std::move(sev));
}

void muxseq_initInstruments () {
    if (! g_cs) {
        return;
    }
    //FIX: emit declarative events (which muxseq can turn into midi-events)
    std::vector<SparseMidiEvent> sevs;
    for (const MidiMapping& mm : g_cs->midiMapping()) {
        const Channel* channel = mm.articulation();
        // g_cs->midiMapping(nev.channel())->articulation()->synti()
        const char* synthName = strdup(channel->synti().isNull() ? "" : qPrintable(channel->synti()));
        for (const MidiCoreEvent& e : channel->initList()) {
            if (e.type() == ME_INVALID) {
                continue;
            }
            add_SparseMidiEvent(sevs, e.type(), channel->channel(), e.dataA(), e.dataB(), synthName);
            LD6("midi-event: type=%i chan=%i dataA=%i dataB=%i name=%s", e.type(), channel->channel(), e.dataA(), e.dataB(), synthName);
        }
        // Setting pitch bend sensitivity to 12 semitones for external synthesizers
        if (mm.channel() != 9) {
            add_SparseMidiEvent(sevs, ME_CONTROLLER, channel->channel(), CTRL_LRPN, 0, synthName);
            add_SparseMidiEvent(sevs, ME_CONTROLLER, channel->channel(), CTRL_HRPN, 0, synthName);
            add_SparseMidiEvent(sevs, ME_CONTROLLER, channel->channel(), CTRL_HDATA,12, synthName);
            add_SparseMidiEvent(sevs, ME_CONTROLLER, channel->channel(), CTRL_LRPN, 127, synthName);
            add_SparseMidiEvent(sevs, ME_CONTROLLER, channel->channel(), CTRL_HRPN, 127, synthName);
        }
    }
    int midiPortCount = g_cs->masterScore()->midiPortCount();
    muxseq_send(MsgTypeMasterSynthInitInstruments, midiPortCount, sevs);
}


MasterScore* muxseq_seq_score () {
    //FIX: return seq3->score();
    return nullptr;
}

void muxseq_seq_set_scoreview (void* v) {
    //FIX: note, this code is from Seq::setScoreView
    if (g_cv != v && g_cs) {
        //unmarkNotes(); // FIX: tracks what notes are being played
        //stopWait();
    }
    g_cv = static_cast<ScoreView*>(v);
    g_cs = g_cv ? g_cv->score()->masterScore() : 0;
    g_midi = MidiRenderer(g_cs);   // see muxseq_client.cpp:maybe_update_midiRenderer()
    g_midi.setMinChunkSize(10);
    //FIX: send to muxseq
    //g_seq->_synti->reset();
    muxseq_initInstruments();
}

void muxseq_seq_setController(int channel, int vol, int iv) {
    //FIX: seq3->setController(channel, vol, iv);
}

void muxseq_seq_updateOutPortCount(int maxPorts) {
    muxseq_send(MsgTypeSeqUpdateOutPortCount, maxPorts);
}

// signals

MuxSeqSig* muxseq_init_muxseqsig() {
    return muxseqsig_init();
}

void muxseq_seq_emit_started () {
    muxseqsig_seq_emit_started();
}

void muxseq_seq_emit_stopped () {
    muxseqsig_seq_emit_stopped();
}

// synthesizer
int muxseq_synthesizerFactory() {
    muxseq_send(MsgTypeMasterSynthesizerInit);
    return 0;
}

//int muxseq_create_synti(int sampleRate) {
//    muxseq_send(MsgTypeMasterSynthesizerInit);
//    return 0; // return some index?
//}

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

MidiPatch* muxseq_synti_getPatchInfo(QString chan, int bank, int prog) {
  return nullptr;
}

QList<MidiPatch*> muxseq_synti_getPatchInfoList () {
  QList<MidiPatch*> pl;
  // FIX: populate list
  return pl;
}


int muxseq_synth_create (int sampleRate, SynthesizerState synthState) {
    muxseq_send(MsgTypeMasterSynthesizerInit);
    //synth->init();
    //synth->setSampleRate(sampleRate);
    //bool r = synth->setState(synthState);
    //if (!r || !synth->hasSoundFontsLoaded()) {
    //    synth->init();
    //}
    //return synth;
    return 0; // FIX: return some synth-index?
}

void muxseq_synth_delete (MasterSynthesizer* synth) {
    //delete synth;
}

Synthesizer* muxseq_synth_get_name(const QString& name) {
    //return synti ? synti->synthesizer(name) : nullptr;
    return nullptr; //FIX: return what?
}

#if 0
void muxseq_synth_load_soundfonts (Synthesizer* s, QStringList sfList) {
    for (auto sf : sfList) {
        s->addSoundFont(sf);
    }
    if (!sfList.isEmpty()) {
        synti->storeState();
    }
    s->gui()->synthesizerChanged();
}
#endif

void muxseq_synth_fluid_load_soundfonts (QStringList sfList) {
    //Synthesizer* s = synti->synthesizer("Fluid");
    //muxseq_synth_load_soundfonts(s, sfList);
}

void muxseq_synth_zerberus_load_soundfonts (QStringList sfzList) {
    //Synthesizer* s = synti->synthesizer("Zerberus");
    //muxseq_synth_load_soundfonts(s, sfzList);
}

#if 0
void muxseq_synth_unload_soundfonts (Synthesizer* s, QStringList sfList) {
    for (auto sf : sfList) {
        s->removeSoundFont(sf);
    }
    if (!sfList.isEmpty()) {
        synti->storeState();
    }
    s->gui()->synthesizerChanged();
}
#endif

void muxseq_synth_fluid_unload_soundfonts (QStringList sfList) {
    //Synthesizer* s = synti->synthesizer("Fluid");
    //muxseq_synth_unload_soundfonts(s, sfList);
}

void muxseq_synth_zerberus_unload_soundfonts (QStringList sfzList) {
    //Synthesizer* s = synti->synthesizer("Zerberus");
    //muxseq_synth_unload_soundfonts(s, sfzList);
}


} // namespace Ms

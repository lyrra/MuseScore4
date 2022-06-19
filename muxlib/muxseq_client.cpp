
#include <iostream>
#include <thread>
#include <chrono>
#include "event.h"
#include "libmscore/synthesizerstate.h"
#include "synthesizer.h"
#include "mux.h"
#include "muxlib.h"
#include "muxseqsig.h"
#include "scoreview.h"
#include "msynthesizer.h"
#include "muxseq_client.h"

namespace Ms {

/* initialization/control
 */
static struct Mux::MuxSocket g_muxseq_query_client_socket;
static struct Mux::MuxSocket g_muxseq_bulletin_client_socket;
static bool g_threads_started = false;
static bool g_thread_musescoreQuery_started = false;
static bool g_thread_musescoreBulletin_started = false;
static std::vector<std::thread> muxseq_Threads;
/**/

#define L_MUX_QUERY(type) \
  qDebug("muxseq_client query %s", muxseq_msg_type_info(type));

int muxseq_query_zmq (MuxseqMsgType type, MuxseqMsg &msg) {
    qDebug("muxseq send msg %s", muxseq_msg_type_info(type));
    msg.type = type;
    mux_zmq_send(g_muxseq_query_client_socket, (void*) &msg, sizeof(struct MuxseqMsg));
    return mux_zmq_recv(g_muxseq_query_client_socket, (void*) &msg, sizeof(struct MuxseqMsg));
}

int muxseq_send (MuxseqMsgType type) {
    struct MuxseqMsg msg;
    return muxseq_query_zmq(type, msg);
}

int muxseq_send (MuxseqMsgType type, int i) {
    struct MuxseqMsg msg;
    msg.payload.i = i;
    qDebug("muxseq-send msg %s, int=%i", muxseq_msg_type_info(type), msg.payload.i);
    return muxseq_query_zmq(type, msg);
}

int muxseq_send (MuxseqMsgType type, double d) {
    struct MuxseqMsg msg;
    msg.payload.d = d;
    qDebug("muxseq-send msg %s, double=%f", muxseq_msg_type_info(type), d);
    return muxseq_query_zmq(type, msg);
}

int muxseq_send (MuxseqMsgType type, NPlayEvent event) {
    struct MuxseqMsg msg;
    muxseq_msg_set_NPlayEvent(msg, event);
    return muxseq_query_zmq(type, msg);
}

void muxseq_query (MuxseqMsgType type) {
    L_MUX_QUERY(type);
    struct MuxseqMsg msg;
    muxseq_query_zmq(type, msg);
}

bool muxseq_query_bool (MuxseqMsgType type) {
    L_MUX_QUERY(type);
    struct MuxseqMsg msg;
    muxseq_query_zmq(type, msg);
    LD("muxseq_query_bool %s => %i", muxseq_msg_type_info(type), msg.payload.b);
    return msg.payload.b;
}

double muxseq_query_float (MuxseqMsgType type) {
    L_MUX_QUERY(type);
    struct MuxseqMsg msg;
    muxseq_query_zmq(type, msg);
    return msg.payload.d;
}

void muxseq_query (MuxseqMsgType type, bool b) {
    L_MUX_QUERY(type);
    qDebug("  -- about bool %i", b);
    struct MuxseqMsg msg;
    msg.payload.b = b;
    muxseq_query_zmq(type, msg);
    return;
}



// void mux_network_close(struct MuxSocket &sock)

void muxseq_query_req_thread_init(std::string _notused)
{
    Mux::mux_make_connection(g_muxseq_query_client_socket, MUX_MUSESCORE_QUERY_CLIENT_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REP, Mux::ZmqServer::CONNECT);
    g_thread_musescoreQuery_started = true;
    //muxseq_network_mainloop_query();
}

void muxseq_query_rep_thread_init(std::string _notused)
{
    Mux::mux_make_connection(g_muxseq_bulletin_client_socket, MUX_MUSESCORE_BULLETIN_CLIENT_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REQ, Mux::ZmqServer::CONNECT);
    g_thread_musescoreBulletin_started = true;
    //muxseq_network_mainloop_bulletin();
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
    while(g_thread_musescoreQuery_started == false ||
          g_thread_musescoreBulletin_started == false) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    g_threads_started = true;
}

/* Musescore/client specific
 *
 */

MasterSynthesizer* synti = 0;

int muxseq_create_synti(int sampleRate);

void muxseq_create(int sampleRate) { // called from musescore.cpp: MuseScore::init
    qDebug("musescore-muxseq-seqCreate sampleRate=%i", sampleRate);
    muxseq_send(MsgTypeSeqCreate, sampleRate);
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

void muxseq_seq_seek(int ticks) {
    muxseq_send(MsgTypeSeqSeek, ticks);
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

void muxseq_seq_initInstruments() {
    //FIX: seq3->initInstruments();
}

void muxseq_preferencesChanged() {
    muxseq_send(MsgTypeSeqPreferencesChanged);
}

MasterScore* muxseq_seq_score () {
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

int muxseq_create_synti(int sampleRate) {
    muxseq_send(MsgTypeMasterSynthesizerInit);
    //synti = muxseq_synthesizerFactory();
    //synti->setSampleRate(sampleRate);
    //synti->init();
    return 0; // return some index?
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

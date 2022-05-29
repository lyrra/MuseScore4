
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

MasterSynthesizer* synti = 0;

int muxseq_create_synti(int sampleRate);

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

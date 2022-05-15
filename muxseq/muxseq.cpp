
#include "muxseqsig.h"
#include "scoreview.h"
#include "audio/midi/event.h"
#include "audio/midi/msynthesizer.h"
#include "effects/zita1/zita.h"
#include "effects/compressor/compressor.h"
#include "effects/noeffect/noeffect.h"
#include "audio/midi/fluid/fluid.h"
#include "audio/midi/synthesizer.h"
#include "audio/midi/synthesizergui.h"
#include "audio/midi/msynthesizer.h"

#ifdef AEOLUS
extern Ms::Synthesizer* createAeolus();
#endif

#ifdef ZERBERUS
extern Ms::Synthesizer* createZerberus();
#endif

namespace Ms {

class MuseScore;

MasterSynthesizer* synti = 0;

#define DEFMUXSEQVOID(name, sname) \
  void muxseq_seq_ ## name() { \
      seq3-> sname (); \
  }

MasterSynthesizer* muxseq_create_synti(int sampleRate);

void muxseq_initialize(int sampleRate) { // called from musescore.cpp: MuseScore::init
    // FIX: query muxaudio about current sampleRate
    MasterSynthesizer* synti = muxseq_create_synti(MScore::sampleRate);
    seq = seq3 = new Seq();
    seq3->setMasterSynthesizer(synti);
}

void muxseq_dealloc() {
    qDebug("!!!! muxseq_dealloc !!!!");
    seq3 = 0;
    seq = 0;
}

void muxseq_exit() {
    seq3->exit();
}

bool muxseq_seq_alive() {
    if (seq == 0 || seq3 == 0) {
        return false;
    }
    return true;
}

bool muxseq_seq_init (bool hotPlug) {
    qDebug("muxseq_seq_init seq=%lx, seq3=%lx", seq, seq3);
    if (! seq || !seq3) {
        qFatal("muxseq_seq_init === WARNING seq/seq3 is not initialized ===");
    }
    return seq3->init(hotPlug);
}

void muxseq_seq_start () {
    seq3->start();
}

void muxseq_seq_stop () {
    seq3->stop();
}

void muxseq_send_event(NPlayEvent event) {
    seq3->sendEvent(event);
}

void muxseq_start_note(int channel, int pitch, int velocity, double nt) {
    seq3->startNote(channel,
                    pitch,
                    velocity,
                    nt);
}

void muxseq_start_note_dur(int channel, int pitch, int velocity, int duration, double nt) {
    seq3->startNote(channel,
                    pitch,
                    velocity,
                    duration,
                    nt);
}

void muxseq_stop_notes () {
    seq3->stopNotes();
}

void muxseq_stop_notes (int channel) {
    seq3->stopNotes(channel);
}

void muxseq_stop_notetimer () {
    seq3->stopNoteTimer();
}

void muxseq_start_notetimer (int duration) {
    seq3->startNoteTimer(duration);
}

void muxseq_stop_wait () {
    seq3->stopWait();
}

bool muxseq_seq_playing() {
    return seq3->isPlaying();
}

bool muxseq_seq_running() {
    return seq3->isRunning();
}

bool muxseq_seq_stopped() {
    return seq3->isStopped();
}

bool muxseq_seq_can_start() {
    return seq3->canStart();
}

void muxseq_seq_seek(int ticks) {
    seq3->seek(ticks);
}

float muxseq_seq_curTempo() {
    return seq3->curTempo();
}

void muxseq_seq_setRelTempo (double tempo) {
    seq3->setRelTempo(tempo);
}

DEFMUXSEQVOID(nextMeasure, nextMeasure)
DEFMUXSEQVOID(nextChord, nextChord)
DEFMUXSEQVOID(prevMeasure, prevMeasure)
DEFMUXSEQVOID(prevChord, prevChord)
DEFMUXSEQVOID(rewindStart, rewindStart)
DEFMUXSEQVOID(seekEnd, seekEnd)
DEFMUXSEQVOID(setLoopIn, setLoopIn);
DEFMUXSEQVOID(setLoopOut, setLoopOut);
DEFMUXSEQVOID(setLoopSelection, setLoopSelection);
DEFMUXSEQVOID(recomputeMaxMidiOutPort, recomputeMaxMidiOutPort);

float muxseq_seq_metronomeGain() {
    return seq3->metronomeGain();
}

void muxseq_seq_playMetronomeBeat(BeatType beatType) {
    seq3->playMetronomeBeat(beatType);
}

void muxseq_seq_initInstruments() {
    seq3->initInstruments();
}

void muxseq_preferencesChanged() {
    seq3->preferencesChanged();
}

MasterScore* muxseq_seq_score () {
    return seq3->score();
}

void muxseq_seq_set_scoreview (void* v) {
    seq3->setScoreView((ScoreView*)v);
}

void muxseq_seq_setController(int channel, int vol, int iv) {
    seq3->setController(channel, vol, iv);
}

void muxseq_seq_updateOutPortCount(int maxPorts) {
    seq3->updateOutPortCount(maxPorts);
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
    synti = muxseq_synthesizerFactory();
    synti->setSampleRate(sampleRate);
    synti->init();
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
    return seq3->synti();
}

void muxseq_synti_init() {
    seq3->synti()->init();
}

float muxseq_synti_getGain () {
    return synti->gain();
}

void muxseq_synti_setSampleRate (float sampleRate) {
    seq3->synti()->setSampleRate(sampleRate);
}

SynthesizerState muxseq_get_synthesizerState() {
    MasterSynthesizer* synti = muxseq_get_synti();
    SynthesizerState state;
    return synti ? synti->state() : state;
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
    s->gui()->synthesizerChanged();
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
    s->gui()->synthesizerChanged();
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


#include "muxseqsig.h"
#include "seq.h"
#include "scoreview.h"
#include "audio/midi/event.h"
#include "audio/midi/msynthesizer.h"

namespace Ms {

class MuseScore;

MasterSynthesizer* synti = 0;

#define DEFMUXSEQVOID(name, sname) \
  void muxseq_seq_ ## name() { \
      seq3-> sname (); \
  }

//extern MScore::Sequencer Mscore::seq;
extern Seq* seq3;
extern Seq* seq;

void* muxseq_alloc() { // called from musescore.cpp: MuseScore::init
    seq = seq3 = new Seq();
    return seq;
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

}     // namespace Ms

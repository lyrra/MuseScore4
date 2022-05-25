#ifndef __MUXSEQCLIENT_H__
#define __MUXSEQCLIENT_H__

#include "midi/midipatch.h"

namespace Ms {

#define DEFMUXSEQVOID(name) void muxseq_seq_ ## name()

void muxseq_initialize(int sampleRate);
void muxseq_dealloc();
void muxseq_exit();
bool muxseq_seq_init(bool hotPlug);
bool muxseq_seq_alive();
void muxseq_send_event(NPlayEvent event);
void muxseq_seq_start();
void muxseq_seq_stop();
void muxseq_start_note(int channel, int pitch, int velocity, double nt);
void muxseq_start_note_dur(int channel, int pitch, int velocity, int duration, double nt);
void muxseq_start_notetimer(int duration);
void muxseq_stop_notes();
void muxseq_stop_notes(int channel);
void muxseq_stop_notetimer();
void muxseq_stop_wait();
bool muxseq_seq_playing();
bool muxseq_seq_running();
bool muxseq_seq_stopped();
bool muxseq_seq_can_start();
void muxseq_seq_seek(int ticks);
int muxseq_seq_curTick();
float muxseq_seq_curTempo();
void muxseq_seq_setRelTempo(double);
DEFMUXSEQVOID(nextMeasure);
DEFMUXSEQVOID(nextChord);
DEFMUXSEQVOID(prevMeasure);
DEFMUXSEQVOID(prevChord);
DEFMUXSEQVOID(rewindStart);
DEFMUXSEQVOID(seekEnd);
DEFMUXSEQVOID(setLoopIn);
DEFMUXSEQVOID(setLoopOut);
DEFMUXSEQVOID(setLoopSelection);
DEFMUXSEQVOID(recomputeMaxMidiOutPort);

float muxseq_seq_metronomeGain();
void muxseq_seq_playMetronomeBeat(BeatType beatType);
void muxseq_seq_initInstruments();
void muxseq_preferencesChanged();
MasterScore* muxseq_seq_score();
void muxseq_seq_set_scoreview(void *v);

void muxseq_seq_setController(int channel, int vol, int iv);
void muxseq_seq_updateOutPortCount(int maxPorts);

int muxseq_create_synti(int sampleRate);
int muxseq_synthesizerFactory();
void muxseq_delete_synti();
bool muxseq_synti();
void muxseq_synti_init();
SynthesizerState muxseq_synti_get_synthesizerState();
void muxseq_synti_setState(SynthesizerState state);
void muxseq_synti_storeState();
float muxseq_synti_getGain();
void muxseq_synti_setGain (double gain);
float muxseq_synti_getMinGainAsDecibels();
float muxseq_synti_getMaxGainAsDecibels();
float muxseq_synti_getGainAsDecibels();
float muxseq_synti_getDefaultGainAsDecibels();
void muxseq_synti_setGainAsDecibels(float gain);
void muxseq_synti_setSampleRate(float sampleRate);
void muxseq_synti_setMasterTuning(float tuning);
float muxseq_synti_getMasterTuning();
void muxseq_synti_setEffect(int effectIdx, int n);
int muxseq_synti_getDynamicsMethod();
void muxseq_synti_setDynamicsMethod(int n);
int muxseq_synti_getCcToUseIndex();
void muxseq_synti_setCcToUseIndex(int n);
int muxseq_synti_getIndexOfEffect(int n);

QList<MidiPatch*> muxseq_synti_getPatchInfoList();
MidiPatch* muxseq_synti_getPatchInfo(QString chan, int bank, int prog);
int muxseq_synth_create (int sampleRate, SynthesizerState synthState);
Synthesizer* muxseq_synth_get_name(const QString& name);
void muxseq_synth_delete (MasterSynthesizer* synth);
void muxseq_synth_fluid_load_soundfonts (QStringList sfList);
void muxseq_synth_zerberus_load_soundfonts (QStringList sfzList);
void muxseq_synth_fluid_unload_soundfonts (QStringList sfList);
void muxseq_synth_zerberus_unload_soundfonts (QStringList sfzList);

} // namespace Ms
#endif

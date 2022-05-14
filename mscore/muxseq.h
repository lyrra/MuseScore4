#ifndef __MUXSEQ_H__
#define __MUXSEQ_H__

namespace Ms {

#define DEFMUXSEQVOID(name) void muxseq_seq_ ## name()

void* muxseq_alloc();
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

MasterSynthesizer* muxseq_create_synti(int sampleRate);
MasterSynthesizer* muxseq_synthesizerFactory();
MasterSynthesizer* muxseq_get_synti();
void muxseq_delete_synti();
bool muxseq_synti();
void muxseq_synti_init();
float muxseq_synti_getGain();
void muxseq_synti_setSampleRate(float sampleRate);
SynthesizerState muxseq_get_synthesizerState();
MasterSynthesizer* muxseq_synth_create (int sampleRate, SynthesizerState synthState);
Synthesizer* muxseq_synth_get_name(const QString& name);
void muxseq_synth_delete (MasterSynthesizer* synth);
void muxseq_synth_fluid_load_soundfonts (QStringList sfList);
void muxseq_synth_zerberus_load_soundfonts (QStringList sfzList);
void muxseq_synth_fluid_unload_soundfonts (QStringList sfList);
void muxseq_synth_zerberus_unload_soundfonts (QStringList sfzList);

} // namespace Ms
#endif

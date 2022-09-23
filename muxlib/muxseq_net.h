
namespace Ms {

int muxseq_query_zmq (MuxseqMsgType type, MuxseqMsg &msg);
int muxseq_send (MuxseqMsgType type);
int muxseq_send (MuxseqMsgType type, int i);
int muxseq_send (MuxseqMsgType type, double d);
int muxseq_send (MuxseqMsgType type, NPlayEvent event);
int muxseq_send (MuxseqMsgType type, int maxMidiPorts, std::vector<struct SparseMidiEvent> sevs);
int muxseq_query (MuxseqMsgType type);
bool muxseq_query_bool (MuxseqMsgType type);
double muxseq_query_float (MuxseqMsgType type);
void muxseq_query (MuxseqMsgType type, bool b);
int mux_query_recv_Muxseq (Mux::MuxSocket &sock, struct MuxseqMsg &msg);
void muxseq_create(int sampleRate);
void muxseq_dealloc();
void muxseq_exit();
bool muxseq_seq_init (bool hotPlug);
void muxseq_seq_start ();
bool muxseq_seq_alive();
void muxseq_stop_notes ();
void muxseq_stop_notes (int channel);
void muxseq_stop_notetimer ();
void muxseq_start_notetimer (int duration);
void muxseq_stop_wait ();
bool muxseq_seq_playing();
bool muxseq_seq_running();
bool muxseq_seq_stopped();
bool muxseq_seq_can_start();
int muxseq_seq_curTick();
float muxseq_seq_curTempo();
void muxseq_seq_setRelTempo (double tempo);
float muxseq_seq_metronomeGain();
void muxseq_seq_playMetronomeBeat(BeatType beatType);
void muxseq_seq_stop ();
void muxseq_send_event(NPlayEvent event);
void muxseq_seq_initInstruments();
void muxseq_preferencesChanged();

#define DEFMUXSEQVOID(name) void muxseq_seq_ ## name()
        
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

}

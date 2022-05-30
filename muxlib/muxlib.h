#ifndef __MUXLIB_H__
#define __MUXLIB_H__

#define MUX_MUSESCORE_QUERY_CLIENT_URL "tcp://localhost:7701"
#define MUX_MUSESCORE_BULLETIN_CLIENT_URL "tcp://localhost:7702"

namespace Ms {

enum MuxseqMsgType {
    MsgTypeNoop = 0,
    MsgTypeSeqInit,
    MsgTypeSeqDeinit,
    MsgTypeSeqExit,
    MsgTypeSeqAlive,
    MsgTypeSeqStart,
    MsgTypeSeqStop,
    MsgTypeSeqSendEvent,
    MsgTypeSeqStartNote,
    MsgTypeSeqStartNoteDur,
    MsgTypeSeqStopNotes,
    MsgTypeSeqStartNoteTimer,
    MsgTypeSeqStopNoteTimer,
    MsgTypeSeqStopWait,
    MsgTypeSeqCurTempo,
    MsgTypeSeqSetRelTempo,
    MsgTypeSeqPlaying,
    MsgTypeSeqRunning,
    MsgTypeSeqStopped,
    MsgTypeSeqCanStart,
    MsgTypeSeqCurTick,
    MsgTypeSeqSeek,
    MsgTypeSeekEnd,
    MsgTypeNextMeasure,
    MsgTypePrevMeasure,
    MsgTypeNextChord,
    MsgTypePrevChord,
    MsgTypeRewindStart,
    MsgTypeSetLoopIn,
    MsgTypeSetLoopOut,
    MsgTypeSetLoopSelection,
    MsgTypeRecomputeMaxMidiOutPort,
    MsgTypeSeqPreferencesChanged,
    MsgTypeSeqUpdateOutPortCount,
    MsgTypeMasterSynthesizerInit,
    MsgTypeEOF
};

//FIX: redundant with muxaudio.h, move to mux.h
struct SparseEvent {
    unsigned char type;
    unsigned char channel;
    int pitch;
    int velo;
    int cont;
    int val;
};
struct SparseMidiEvent {
    unsigned int framepos;
    int portIdx;
    int channel;
    unsigned char type;
    int dataA;
    int dataB;
};

struct MuxseqMsg {
    MuxseqMsgType type;
    union Payload {
        int i;
        bool b;
        double d;
        SparseEvent sparseEvent;
        SparseMidiEvent sparseMidiEvent;
    } payload;
};

int  muxseq_send (MuxseqMsgType type);
int  muxseq_send (MuxseqMsgType type, int i);
int  muxseq_send (MuxseqMsgType type, double d);
int  muxseq_send (MuxseqMsgType type, NPlayEvent event);
void muxseq_query (MuxseqMsgType type);
bool muxseq_query_bool (MuxseqMsgType type);
double muxseq_query_float (MuxseqMsgType type);
void muxseq_query (MuxseqMsgType type, bool b);
void mux_musescore_client_start ();

} // namespace Ms
#endif

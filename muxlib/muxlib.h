#ifndef __MUXLIB_H__
#define __MUXLIB_H__

#define MUX_MUSESCORE_QUERY_CLIENT_URL "tcp://localhost:7701"
#define MUX_MUSESCORE_BULLETIN_CLIENT_URL "tcp://localhost:7702"

namespace Ms {

enum MsgType {
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

struct MuxseqMsg {
    int type;
    union payload {
        int i;
        bool b;
        NPlayEvent event;
    };
};

void muxseq_send(MsgType type);
void muxseq_send(MsgType type, int i);
void muxseq_send(MsgType type, double d);
void muxseq_send(MsgType type, NPlayEvent event);
void muxseq_query(MsgType type);
bool muxseq_query_bool(MsgType type);
float muxseq_query_float(MsgType type);
void muxseq_query(MsgType type, bool b);
void mux_musescore_client_start();

} // namespace Ms
#endif

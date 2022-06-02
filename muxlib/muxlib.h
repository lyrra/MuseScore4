#ifndef __MUXLIB_H__
#define __MUXLIB_H__

#define LD(...) qDebug(__VA_ARGS__);
#define LE(...) qFatal(__VA_ARGS__);

#define MUX_MUSESCORE_QUERY_CLIENT_URL "tcp://localhost:7701"
#define MUX_MUSESCORE_BULLETIN_CLIENT_URL "tcp://localhost:7702"
#define MUX_MUSESCORE_QUERY_SERVER_URL "tcp://*:7701"
#define MUX_MUSESCORE_BULLETIN_SERVER_URL "tcp://*:7702"
#define MUX_AUDIO_QUERY_CLIENT_URL "tcp://localhost:7711"

namespace Ms {

enum MuxseqMsgType {
    MsgTypeNoop = 0,
    MsgTypeSeqCreate,
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

enum MuxaudioMsgType {
    MsgTypeInit = 0,
    MsgTypeAudioInit,
    MsgTypeAudioStart,
    MsgTypeAudioStop,
    MsgTypeAudioBufferFeed,
    MsgTypeAudioRunning,
    MsgTypeTransportStart,
    MsgTypeTransportStop,
    MsgTypeTransportSeek,
    MsgTypeJackTransportPosition,
    MsgTypeEventToGui,
    MsgTypeEventToMidi,
    MsgTypeTimeSigTempoChanged,
    MsgTypeOutPortCount,
    MsgTypeEOF2
};

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

struct JackTransportPosition {
    unsigned int state;
    unsigned int frame;
    unsigned int valid;
    unsigned int beats_per_minute;
    unsigned int bbt;
};

struct MuxaudioMsg {
    MuxaudioMsgType type;
    union Payload {
        int i;
        SparseEvent sparseEvent;
        SparseMidiEvent sparseMidiEvent;
        struct JackTransportPosition jackTransportPosition;
    } payload;
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

const char*   muxseq_msg_type_info (MuxseqMsgType   type);
const char* muxaudio_msg_type_info (MuxaudioMsgType type);
void muxseq_msg_set_NPlayEvent (MuxseqMsg msg, NPlayEvent event);
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

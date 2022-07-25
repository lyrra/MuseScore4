#ifndef __MUXLIB_H__
#define __MUXLIB_H__


#define MUX_MUSESCORE_QUERY_CLIENT_URL "tcp://localhost:7701"
#define MUX_MUSESCORE_QUERYREQ_CLIENT_URL "tcp://localhost:7704"
#define MUX_MUSESCORE_BULLETIN_CLIENT_URL "tcp://localhost:7702"
#define MUX_MUSESCORE_QUERY_SERVER_URL "tcp://*:7701"
#define MUX_MUSESCORE_QUERYREQ_SERVER_URL "tcp://*:7704"
#define MUX_MUSESCORE_BULLETIN_SERVER_URL "tcp://*:7702"
#define MUX_MUXAUDIO_QUERY_AUDIO_CLIENT_URL "tcp://localhost:7712"
#define MUX_MUXAUDIO_QUERY_CTRL_CLIENT_URL "tcp://localhost:7711"

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
    MsgTypeSeqStarted,
    MsgTypeSeqStopped,
    MsgTypeSeqCanStart,
    MsgTypeSeqCurTick,
    MsgTypeSeqUTick,
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
    MsgTypeSeqRenderEvents,
    MsgTypeMasterSynthesizerInit,
    MsgTypeMasterSynthInitInstruments,
    MsgTypeEOF
};

enum MuxaudioMsgType {
    MsgTypeAudioNoop = 0,
    MsgTypeInit,
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
    unsigned int framepos;
    unsigned char type;
    unsigned char channel;
    char synthName[64];
    int midiPort;
    int pitch;
    int velo;
    int beatsPerSecond;
    int ticksPerSecond;
    double playPosSeconds;
    int division;
};

struct SparseMidiEvent {
    unsigned int framepos;
    char synthName[64];
    int midiPort;
    int channel;
    int dataA;
    int dataB;
    unsigned char type;
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
        struct SparseEvent sparseEvent;
        struct SparseMidiEvent sparseMidiEvent;
        struct JackTransportPosition jackTransportPosition;
    } payload;
};

struct MuxseqMsg {
    MuxseqMsgType type;
    char label[64];
    union Payload {
        int i;
        bool b;
        double d;
        SparseEvent sparseEvent;
    } payload;
};

struct MuxseqEventsHeader {
    int type;
    int numEvents;
    struct SparseEvent *sevs;
};

#define MUX_CHUNK_NUMFLOATS 512

struct MuxaudioBuffer {
    uint64_t utick;
    uint8_t flags[8];
    // minimal amount of work considered feasible (performance-wise)
    float buf[MUX_CHUNK_NUMFLOATS];
};

/* both client and server needs to use the same chunk-size, so define it once, here */
#define MUX_CHUNKSIZE_NET sizeof(struct MuxaudioBuffer)

const char*   muxseq_msg_type_info (MuxseqMsgType   type);
const char* muxaudio_msg_type_info (MuxaudioMsgType type);
void muxseq_msg_set_NPlayEvent (MuxseqMsg msg, NPlayEvent event);
int  muxseq_send (MuxseqMsgType type);
int  muxseq_send (MuxseqMsgType type, int i);
int  muxseq_send (MuxseqMsgType type, double d);
int  muxseq_send (MuxseqMsgType type, NPlayEvent event);
int  muxseq_query (MuxseqMsgType type);
bool muxseq_query_bool (MuxseqMsgType type);
double muxseq_query_float (MuxseqMsgType type);
void muxseq_query (MuxseqMsgType type, bool b);
void mux_musescore_client_start ();

} // namespace Ms
#endif

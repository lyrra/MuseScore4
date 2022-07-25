
#include "event.h"
#include "mux.h"
#include "muxlib.h"

namespace Ms {

extern void _logstr (char *str);
thread_local char _logbuf[256];


int overlap_strcat(char* dst, char* src) {
    int i = 0, a = 0, f = 0;
    while (1) {
        if (! f) {
            if (dst[i] == 0) {
                f = 1; // found null-terminator
            }
        }
        if (f) {
            dst[i] = src[a++];
            if (src[a] == 0) {
                dst[++i] = 0;
                i++;
                break;
            }
        }
        i++;
    }
    return i;
}

int get_timestamp (char *buf) {
    struct timespec tp;
    buf[0] = 0;
    uint64_t d = 0;
    if (! clock_gettime(CLOCK_MONOTONIC, &tp)) {
        d = (tp.tv_sec & 0xfff) * 1000000;
        d += tp.tv_nsec / 1000;
    }
    sprintf(buf, "%li ", d);
}

void _log_write () {
    _logbuf[255] = 0; // ambiguous snprintf standard definition
    char t[64+256];
    get_timestamp(t);
    int n = overlap_strcat(t, _logbuf);
    if (n > 0) {
        _logstr(t);
    }
//    fflush(stdout);
//    int fd = fileno(stdout);
//#ifdef WIN32
//  _commit(fd);
//#else
//  fsync(fd);
//#endif
}

const char* muxseq_msg_type_info (MuxseqMsgType type) {
    switch (type) {
    case MsgTypeNoop: return "MsgTypeNoop";
    case MsgTypeSeqCreate: return "MsgTypeSeqCreate";
    case MsgTypeSeqInit: return "MsgTypeSeqInit";
    case MsgTypeSeqDeinit: return "MsgTypeSeqDeinit";
    case MsgTypeSeqExit: return "MsgTypeSeqExit";
    case MsgTypeSeqAlive: return "MsgTypeSeqAlive";
    case MsgTypeSeqStart: return "MsgTypeSeqStart";
    case MsgTypeSeqStop: return "MsgTypeSeqStop";
    case MsgTypeSeqSendEvent: return "MsgTypeSeqSendEvent";
    case MsgTypeSeqStartNote: return "MsgTypeSeqStartNote";
    case MsgTypeSeqStartNoteDur: return "MsgTypeSeqStartNoteDur";
    case MsgTypeSeqStopNotes: return "MsgTypeSeqStopNotes";
    case MsgTypeSeqStartNoteTimer: return "MsgTypeSeqStartNoteTimer";
    case MsgTypeSeqStopNoteTimer: return "MsgTypeSeqStopNoteTimer";
    case MsgTypeSeqStopWait: return "MsgTypeSeqStopWait";
    case MsgTypeSeqCurTempo: return "MsgTypeSeqCurTempo";
    case MsgTypeSeqSetRelTempo: return "MsgTypeSeqSetRelTempo";
    case MsgTypeSeqPlaying: return "MsgTypeSeqPlaying";
    case MsgTypeSeqRunning: return "MsgTypeSeqRunning";
    case MsgTypeSeqStarted: return "MsgTypeSeqStarted";
    case MsgTypeSeqStopped: return "MsgTypeSeqStopped";
    case MsgTypeSeqCanStart: return "MsgTypeSeqCanStart";
    case MsgTypeSeqCurTick: return "MsgTypeSeqCurTick";
    case MsgTypeSeqUTick: return "MsgTypeSeqUTick";
    case MsgTypeSeqSeek: return "MsgTypeSeqSeek";
    case MsgTypeSeekEnd: return "MsgTypeSeekEnd";
    case MsgTypeNextMeasure: return "MsgTypeNextMeasure";
    case MsgTypePrevMeasure: return "MsgTypePrevMeasure";
    case MsgTypeNextChord: return "MsgTypeNextChord";
    case MsgTypePrevChord: return "MsgTypePrevChord";
    case MsgTypeRewindStart: return "MsgTypeRewindStart";
    case MsgTypeSetLoopIn: return "MsgTypeSetLoopIn";
    case MsgTypeSetLoopOut: return "MsgTypeSetLoopOut";
    case MsgTypeSetLoopSelection: return "MsgTypeSetLoopSelection";
    case MsgTypeRecomputeMaxMidiOutPort: return "MsgTypeRecomputeMaxMidiOutPort";
    case MsgTypeSeqPreferencesChanged: return "MsgTypeSeqPreferencesChanged";
    case MsgTypeSeqUpdateOutPortCount: return "MsgTypeSeqUpdateOutPortCount";
    case MsgTypeSeqRenderEvents: return "MsgTypeSeqRenderEvents";
    case MsgTypeMasterSynthesizerInit: return "MsgTypeMasterSynthesizerInit";
    case MsgTypeMasterSynthInitInstruments: return "MsgTypeMasterSynthInitInstruments";
    case MsgTypeEOF: return "MsgTypeEOF";
    default: return "ERROR:UNKNOWN-MUXSEQ-MSG-TYPE";
    }
}

const char* muxaudio_msg_type_info(MuxaudioMsgType type) {
    switch (type) {
        case MsgTypeAudioInit: return "MsgTypeAudioInit";
        case MsgTypeAudioStart: return "MsgTypeAudioStart";
        case MsgTypeAudioStop: return "MsgTypeAudioStop";
        case MsgTypeAudioRunning: return "MsgTypeAudioRunning";
        case MsgTypeJackTransportPosition: return "MsgTypeJackTransportPosition";
        case MsgTypeTransportStart: return "MsgTypeTransportStart";
        case MsgTypeTransportStop: return "MsgTypeTransportStop";
        case MsgTypeTransportSeek: return "MsgTypeTransportSeek";
        case MsgTypeEventToMidi: return "MsgTypeEventToMidi";
        case MsgTypeEventToGui: return "MsgTypeEventToGui";
        case MsgTypeTimeSigTempoChanged: return "MsgTypeTimeSigTempoChanged";
        case MsgTypeOutPortCount: return "MsgTypeOutPortCount";
        default: return "ERROR:UNKNOWN-MUXAUDIO-MSG-TYPE";
    }
}

void muxseq_msg_set_NPlayEvent (MuxseqMsg msg, NPlayEvent event) {
    msg.payload.sparseEvent.type    = event.type();
    msg.payload.sparseEvent.channel = event.channel();
    msg.payload.sparseEvent.pitch   = event.pitch();
    msg.payload.sparseEvent.velo    = event.velo();
}


} // namespace Ms

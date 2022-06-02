
#include "event.h"
#include "mux.h"
#include "muxlib.h"

namespace Ms {

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
    case MsgTypeSeqStopped: return "MsgTypeSeqStopped";
    case MsgTypeSeqCanStart: return "MsgTypeSeqCanStart";
    case MsgTypeSeqCurTick: return "MsgTypeSeqCurTick";
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
    case MsgTypeMasterSynthesizerInit: return "MsgTypeMasterSynthesizerInit";
    case MsgTypeEOF: return "MsgTypeEOF";
    default: return "ERROR:UNKNOWN-MUXSEQ-MSG-TYPE";
    }
}

const char* muxaudio_msg_type_info(MuxaudioMsgType type) {
    switch (type) {
        case MsgTypeAudioInit: return "MsgTypeAudioInit";
        case MsgTypeAudioStart: return "MsgTypeAudioStart";
        case MsgTypeAudioStop: return "MsgTypeAudioStop";
        case MsgTypeTransportStart: return "MsgTypeTransportStart";
        case MsgTypeTransportStop: return "MsgTypeTransportStop";
        case MsgTypeTransportSeek: return "MsgTypeTransportSeek";
        case MsgTypeEventToMidi: return "MsgTypeEventToMidi";
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
    msg.payload.sparseEvent.cont    = event.controller();
    msg.payload.sparseEvent.val     = event.value();
}


} // namespace Ms

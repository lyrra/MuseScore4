
#include "event.h"
#include "mux.h"
#include "muxlib.h"

namespace Ms {

const char* mux_msg_type_info (MsgType type) {
    switch (type) {
    case MsgTypeNoop: return "MsgTypeNoop";
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
    default: return "ERROR:UNKNOWN-MUX-MSG-TYPE";
    }
}

#define L_MUX_QUERY(type) \
  qDebug("muxseq_client query %s", mux_msg_type_info(type));

void muxseq_send(MsgType type) {
    qDebug("muxseq send msg %s", mux_msg_type_info(type));
}

void muxseq_send(MsgType type, int i) {
    qDebug("muxseq msg %i about int %i", type, i);
}
void muxseq_send(MsgType type, double d) {
    qDebug("muxseq msg %i about int %f", type, d);
}

void muxseq_send(MsgType type, NPlayEvent event) {
    qDebug("muxseq msg %i about event", type);
}

void muxseq_query(MsgType type) {
    L_MUX_QUERY(type);
}

bool muxseq_query_bool(MsgType type) {
    L_MUX_QUERY(type);
    return true;
}

float muxseq_query_float(MsgType type) {
    qDebug("muxseq msg query %i", type);
    return 0.0f;
}

void muxseq_query(MsgType type, bool b) {
    L_MUX_QUERY(type);
    qDebug("  -- about bool %i", b);
}

/*
 * controller for muxseq client
 *
 */

static bool g_threads_started = false;

static std::vector<std::thread> muxseq_Threads;

static struct Mux::MuxSocket g_muxseq_query_client_socket;
static struct Mux::MuxSocket g_muxseq_bulletin_client_socket;

/**/
// void mux_network_close(struct MuxSocket &sock)

void muxseq_query_client_thread_init(std::string _notused)
{
    Mux::mux_network_query_client(g_muxseq_query_client_socket, "MUX_MUSESCORE_QUERY_CLIENT_URL", true);
    //muxseq_network_mainloop_query();
}

void muxseq_bulletin_client_thread_init(std::string _notused)
{
    Mux::mux_network_bulletin_client(g_muxseq_bulletin_client_socket, "MUX_MUSESCORE_BULLETIN_CLIENT_URL");
    //muxseq_network_mainloop_bulletin();
}

void mux_musescore_client_start()
{
    if (g_threads_started) {
        qWarning("musescore-mux-client already started");
        return;
    }
    g_threads_started = true;
    std::vector<std::thread> threadv;
    //
    std::thread zmqMuxseqQueryThread(muxseq_query_client_thread_init, "notused");
    threadv.push_back(std::move(zmqMuxseqQueryThread));
    //
    std::thread zmqMuxseqBulletinThread(muxseq_bulletin_client_thread_init, "notused");
    threadv.push_back(std::move(zmqMuxseqBulletinThread));
    // move threads to heap
    muxseq_Threads = std::move(threadv);
}


} // namespace Ms

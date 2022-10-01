#include "event.h"
#include "mux.h"
#include "muxcommon.h"
#include "muxlib.h"
#include "muxseqlib.h"

namespace Ms {

static struct Mux::MuxSocket g_libmscore_muxseq_query_client_socket;

void _logstr (char *str) {
    qDebug(str);
}

int muxseq_query_zmq (MuxseqMsgType type, MuxseqMsg &msg) {
    msg.type = type;
    Mux::mux_zmq_send(g_libmscore_muxseq_query_client_socket, (void*) &msg, sizeof(struct MuxseqMsg));
    return Mux::mux_zmq_recv(g_libmscore_muxseq_query_client_socket, (void*) &msg, sizeof(struct MuxseqMsg));
}

#if 0
void muxseq_send_event(NPlayEvent event) {
    muxseq_send(MsgTypeSeqSendEvent, event);
}
int muxseq_send (MuxseqMsgType type) {
    struct MuxseqMsg msg;
    return muxseq_query_zmq(type, msg);
}

int muxseq_send (MuxseqMsgType type, NPlayEvent event) {
    struct MuxseqMsg msg;
    muxseq_msg_set_NPlayEvent(msg, event);
    return muxseq_query_zmq(type, msg);
}
#endif

int muxseq_query (MuxseqMsgType type) {
    //L_MUX_QUERY(type);
    struct MuxseqMsg msg;
    muxseq_query_zmq(type, msg);
    return 0;
}

#if 0
void muxseq_start_note(int channel, int pitch, int velocity, double nt) {
    muxseq_send(MsgTypeSeqStartNote);
}

bool muxseq_seq_alive() {
    //FIX: perhaps locally cache this?
    muxseq_query(MsgTypeSeqAlive);
    return true;
}
#endif

}


#ifndef __MUX_H__
#define __MUX_H__


namespace Ms {

/*** MUX public interface ***/
int mux_is_score_open ();
void mux_send_event (Event e);
void mux_process_bufferStereo(unsigned int numFrames, float* bufferStereo);
void mux_set_jack_transport(Transport transport);
/****************************/

/*
 * message queue, between audio and mux
 */

enum MsgType {
    MsgTypeInit = 0,
    MsgTypeTransportStart,
    MsgTypeTransportStop,
    MsgTypeJackTransportPosition,
    MsgTypeEventToGui,
    MsgTypeEventToMidi,
    MsgTypeNoop
};

struct JackTransportPosition {
    unsigned int frame;
    unsigned int valid;
    unsigned int beats_per_minute;
    unsigned int bbt;
};

struct SparseEvent {
    uchar type;
    uchar channel;
    int pitch;
    int velo;
    int cont;
    int val;
};

struct SparseMidiEvent {
    unsigned int framepos;
    int portIdx;
    int channel;
    uchar type;
    int dataA;
    int dataB;
};

struct Msg {
    MsgType type;
    union Payload {
        int i;
        SparseEvent sparseEvent;
        SparseMidiEvent sparseMidiEvent;
        struct JackTransportPosition jackTransportPosition;
    } payload;
};

int mux_mq_from_audio_writer_put (struct Msg msg);
int mux_mq_to_audio_writer_put (struct Msg msg);
int mux_mq_to_audio_visit();

void mux_set_jack_position(struct JackTransportPosition jackTransportPosition);
void mux_audio_jack_transport_start();
void mux_audio_jack_transport_stop();

} // namespace Ms
#endif

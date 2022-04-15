
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
    MsgTypeNoop
};

struct JackTransportPosition {
    unsigned int frame;
    unsigned int valid;
    unsigned int beats_per_minute;
    unsigned int bbt;
};

struct Msg {
    MsgType type;
    union Payload {
        int i;
        struct JackTransportPosition jackTransportPosition;
    } payload;
};

int mux_mq_from_audio_writer_put (struct Msg msg);

void mux_set_jack_position(struct JackTransportPosition jackTransportPosition);

} // namespace Ms
#endif


#ifndef __MUX_H__
#define __MUX_H__


namespace Ms {

/*** MUX public interface ***/
int mux_is_score_open ();
void mux_send_event (Event e);
void mux_process_bufferStereo(unsigned int numFrames, float* bufferStereo);
void mux_set_jack_position(unsigned int frame,
                           unsigned int valid,
                           unsigned int beats_per_minute,
                           unsigned int bbt);
void mux_set_jack_transport(Transport transport);
/****************************/

/*
 * message queue, between audio and mux
 */

enum MsgType {
    MsgTransportStart,
    MsgTransportStop,
    MsgInit
};

struct Msg {
    int init;
    MsgType type;
    union {
        int i;
    } Payload;
};

} // namespace Ms
#endif

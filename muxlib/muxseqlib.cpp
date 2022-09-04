
#include "event.h"

namespace Ms {

class MuseScore;

bool muxseq_seq_alive() {
    //if (seq == 0) {
    //    return false;
    //}
    //FIX: send a query to muxseq-zmq
    return true;
}

void muxseq_send_event(NPlayEvent event) {
    //FIX: send the event over muxseq-zmq
    //seq->sendEvent(event);
}

void muxseq_start_note(int channel, int pitch, int velocity, double nt) {
    //FIX: send the event over muxseq-zmq
    //seq->startNote(channel,
    //               pitch,
    //               velocity,
    //               nt);
}

} // namespace Ms

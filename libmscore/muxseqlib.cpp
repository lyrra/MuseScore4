
#include "../mscore/seq.h"

namespace Ms {

class MuseScore;

extern Seq* seq;

bool muxseq_seq_alive() {
    if (seq == 0) {
        return false;
    }
    return true;
}

void muxseq_send_event(NPlayEvent event) {
    seq->sendEvent(event);
}

void muxseq_start_note(int channel, int pitch, int velocity, double nt) {
    seq->startNote(channel,
                   pitch,
                   velocity,
                   nt);
}

} // namespace Ms


#include "../mscore/seq.h"

namespace Ms {

class MuseScore;

#define DEFMUXSEQVOID(name, sname) \
  void muxseq_seq_ ## name() { \
      seq3-> sname (); \
  }

extern Seq* seq3;
extern Seq* seq;

bool muxseq_seq_alive() {
    if (seq == 0 || seq3 == 0) {
        return false;
    }
    return true;
}

void muxseq_send_event(NPlayEvent event) {
    seq3->sendEvent(event);
}

void muxseq_start_note(int channel, int pitch, int velocity, double nt) {
    seq3->startNote(channel,
                    pitch,
                    velocity,
                    nt);
}


} // namespace Ms

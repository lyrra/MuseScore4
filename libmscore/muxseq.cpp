
#include "mscore.h"
//#include "sequencer.h"
#include "../mscore/seq.h"
#include "audio/midi/event.h"

namespace Ms {

//extern MScore::Sequencer Mscore::seq;

void muxseq_init(void* seq) {
    MScore::seq2 = (Ms::Sequencer*) seq;
}

void muxseq_deinit() {
    MScore::seq2 = 0;
}

bool muxseq_seq_alive() {
    if (MScore::seq2 == 0) {
        return false;
    }
    return true;
}

void muxseq_send_event(NPlayEvent event) {
    MScore::seq2->sendEvent(event);
}

void muxseq_start_note(int channel, int pitch, int velocity, double nt) {
    MScore::seq2->startNote(channel,
                           pitch,
                           velocity,
                           nt);
}

}     // namespace Ms

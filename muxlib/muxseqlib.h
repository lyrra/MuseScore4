#ifndef __MUXSEQLIB_H__
#define __MUXSEQLIB_H__

namespace Ms {

bool muxseq_seq_alive();
void muxseq_send_event(NPlayEvent event);
void muxseq_start_note(int channel, int pitch, int velocity, double nt);

} // namespace Ms
#endif

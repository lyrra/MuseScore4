#ifndef __MUXSEQ_H__
#define __MUXSEQ_H__

namespace Ms {

void muxseq_init(void* seq);
void muxseq_deinit();
bool muxseq_seq_alive();
void muxseq_send_event(NPlayEvent event);
void muxseq_start_note(int channel, int pitch, int velocity, double nt);

}     // namespace Ms
#endif

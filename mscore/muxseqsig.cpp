
#include "seq.h"
#include "muxseqsig.h"
#include "musescore.h"

namespace Ms {

void handle_seqStarted();
void handle_seqStopped();

extern MuseScore* mscore;
MuxSeqSig* muxseqsig;
extern Seq* seq3;

MuxSeqSig* muxseqsig_init() {
  muxseqsig = new MuxSeqSig();
  return muxseqsig;
}

MuxSeqSig* muxseqsig_get() {
  return muxseqsig;
}

MuxSeqSig::~MuxSeqSig()
{
    qDebug("!!!! WARNING MUXSEQSIG is destructed !!!!");
}

void MuxSeqSig::emit_sigSeqStarted() {
    emit sigSeqStarted();
}

void MuxSeqSig::emit_sigSeqStopped() {
    emit sigSeqStopped();
}

void muxseqsig_seq_emit_started () {
    muxseqsig->emit_sigSeqStarted();
}

void muxseqsig_seq_emit_stopped () {
    muxseqsig->emit_sigSeqStopped();
}

// signal handlers
void MuxSeqSig::sigSeqStarted() {
    mscore->seqStarted();
}

void MuxSeqSig::sigSeqStopped() {
    mscore->seqStopped();
}

void MuxSeqSig::setMetronomeGain(float gain) {
    seq3->setMetronomeGain(gain);
}

void MuxSeqSig::setRelTempo(double tempo) {
    seq3->setRelTempo(tempo);
}

void MuxSeqSig::seek(int pos) {
    seq3->seek(pos);
}

}

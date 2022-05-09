
#include "muxseqsig.h"
#include "musescore.h"

namespace Ms {

void handle_seqStarted();
void handle_seqStopped();

extern MuseScore* mscore;
MuxSeqSig* muxseqsig;

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

}

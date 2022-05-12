
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

// object signal emitters

void MuxSeqSig::emit_sigSeqStarted() {
    emit sigSeqStarted();
}

void MuxSeqSig::emit_sigSeqStopped() {
    emit sigSeqStopped();
}

void MuxSeqSig::emit_gainChanged(float gain) {
    emit gainChanged(gain);
}

// functional signal emitters

void muxseqsig_seq_emit_started () {
    muxseqsig->emit_sigSeqStarted();
}

void muxseqsig_seq_emit_stopped () {
    muxseqsig->emit_sigSeqStopped();
}

void muxseqsig_emit_gainChanged (float gain) {
    muxseqsig->emit_gainChanged(gain);
}

// signal handlers

void MuxSeqSig::sigSeqStartedHandle() {
    mscore->seqStarted();
}

void MuxSeqSig::sigSeqStoppedHandle() {
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

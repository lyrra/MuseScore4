
#include "musescore.h"
#include "scoreview.h"
#include "muxseqsig.h"
#include "muxcommon.h"

namespace Ms {

extern MuseScore* mscore;
extern MasterScore* g_cs;
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

// object signal emitters

void MuxSeqSig::emit_sigSeqStarted(unsigned int utick) {
    emit sigSeqStarted(utick);
}

void MuxSeqSig::emit_sigSeqStopped(unsigned int utick) {
    emit sigSeqStopped(utick);
}

void MuxSeqSig::emit_sigSeqUTick(unsigned int utick) {
    emit sigSeqUTick(utick);
}

void MuxSeqSig::emit_gainChanged(float gain) {
    emit gainChanged(gain);
}

// functional signal emitters

void muxseqsig_seq_emit_started (uint64_t utick) {
    muxseqsig->emit_sigSeqStarted(utick);
}

void muxseqsig_seq_emit_stopped (uint64_t utick) {
    muxseqsig->emit_sigSeqStopped(utick);
}

void muxseqsig_seq_emit_utick (uint64_t utick) {
    muxseqsig->emit_sigSeqUTick(utick);
}

void muxseqsig_emit_gainChanged (float gain) {
    muxseqsig->emit_gainChanged(gain);
}

// signal handlers

void MuxSeqSig::sigSeqStartedHandle(unsigned int utick) {
    mscore->seqStarted(utick);
}

void MuxSeqSig::sigSeqStoppedHandle(unsigned int utick) {
    mscore->seqStopped(utick);
}

void MuxSeqSig::sigSeqUTickHandle(unsigned int utick) {
    mscore->handleUTick(utick);
}

void MuxSeqSig::setMetronomeGain(float gain) {
    //FIX: seq3->setMetronomeGain(gain);
}

void MuxSeqSig::setRelTempo(double tempo) {
    //FIX: seq3->setRelTempo(tempo);
}

void MuxSeqSig::seek(int pos) {
    //FIX: seq3->seek(pos);
}

}

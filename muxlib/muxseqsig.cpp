
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

void MuxSeqSig::emit_sigSeqStarted(unsigned int playframe) {
    emit sigSeqStarted(playframe);
}

void MuxSeqSig::emit_sigSeqStopped(unsigned int playframe) {
    emit sigSeqStopped(playframe);
}

void MuxSeqSig::emit_sigSeqUTick(unsigned int tick) {
    qDebug("MuxSeqSig::emit_sigSeqUTick tick=%i", tick);
    emit sigSeqUTick(tick);
}

void MuxSeqSig::emit_gainChanged(float gain) {
    emit gainChanged(gain);
}

// functional signal emitters

void muxseqsig_seq_emit_started (uint64_t playframe) {
    muxseqsig->emit_sigSeqStarted(playframe);
}

void muxseqsig_seq_emit_stopped (uint64_t playframe) {
    muxseqsig->emit_sigSeqStopped(playframe);
}

void muxseqsig_seq_emit_utick (uint64_t tick) {
    qDebug("muxseqsig_seq_emit_utick tick=%i", tick);
    muxseqsig->emit_sigSeqUTick(tick);
}

void muxseqsig_emit_gainChanged (float gain) {
    muxseqsig->emit_gainChanged(gain);
}

// signal handlers

void MuxSeqSig::sigSeqStartedHandle(unsigned int playframe) {
    mscore->seqStarted(playframe);
}

void MuxSeqSig::sigSeqStoppedHandle(unsigned int playframe) {
    mscore->seqStopped(playframe);
}

void MuxSeqSig::sigSeqUTickHandle(unsigned int utick) {
    LD("MSCORE handle utick: %ld", utick);
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

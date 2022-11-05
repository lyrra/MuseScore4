#ifndef __MUXSEQSIG_H__
#define __MUXSEQSIG_H__

namespace Ms {

class MuxSeqSig : public QObject {
    Q_OBJECT

    public:
        ~MuxSeqSig();
        void emit_sigSeqStarted(unsigned int utick);
        void emit_sigSeqStopped(unsigned int utick);
        void emit_sigSeqUTick(unsigned int utick);
        void emit_gainChanged(float gain);
    public slots:
        void setMetronomeGain(float gain);
        void setRelTempo(double tempo);
        void seek(int pos);
        void sigSeqStartedHandle(unsigned int utick);
        void sigSeqStoppedHandle(unsigned int utick);
        void sigSeqUTickHandle(unsigned int utick);

    signals:
        void sigSeqStarted(unsigned int utick);
        void sigSeqStopped(unsigned int utick);
        void sigSeqUTick(unsigned int utick);
        void gainChanged(float); // MasterSynthesizer
};

MuxSeqSig* muxseqsig_init();
MuxSeqSig* muxseqsig_get();
void muxseqsig_seq_emit_started(uint64_t utick);
void muxseqsig_seq_emit_stopped(uint64_t utick);
void muxseqsig_seq_emit_utick(uint64_t utick);
void muxseqsig_emit_gainChanged (float gain);

} // namespace Ms
#endif

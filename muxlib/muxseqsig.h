#ifndef __MUXSEQSIG_H__
#define __MUXSEQSIG_H__

namespace Ms {

class MuxSeqSig : public QObject {
    Q_OBJECT

    public:
        ~MuxSeqSig();
        void emit_sigSeqStarted(unsigned int playframe);
        void emit_sigSeqStopped(unsigned int playframe);
        void emit_sigSeqUTick(unsigned int tick);
        void emit_gainChanged(float gain);
    public slots:
        void setMetronomeGain(float gain);
        void setRelTempo(double tempo);
        void seek(int pos);
        void sigSeqUTickHandle(unsigned int tick);
        void sigSeqStartedHandle(unsigned int playframe);
        void sigSeqStoppedHandle(unsigned int playframe);

    signals:
        void sigSeqStarted(unsigned int playframe);
        void sigSeqStopped(unsigned int playframe);
        void sigSeqUTick(unsigned int tick);
        void gainChanged(float); // MasterSynthesizer
};

MuxSeqSig* muxseqsig_init();
MuxSeqSig* muxseqsig_get();
void muxseqsig_seq_emit_started(uint64_t playframe);
void muxseqsig_seq_emit_stopped(uint64_t playframe);
void muxseqsig_seq_emit_utick(uint64_t tick);
void muxseqsig_emit_gainChanged (float gain);

} // namespace Ms
#endif

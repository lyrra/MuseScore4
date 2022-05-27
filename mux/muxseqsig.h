#ifndef __MUXSEQSIG_H__
#define __MUXSEQSIG_H__

namespace Ms {

class MuxSeqSig : public QObject {
    Q_OBJECT

    public:
        ~MuxSeqSig();
        void emit_sigSeqStarted();
        void emit_sigSeqStopped();
        void emit_gainChanged(float gain);
    public slots:
        void setMetronomeGain(float gain);
        void setRelTempo(double tempo);
        void seek(int pos);
        void sigSeqStartedHandle();
        void sigSeqStoppedHandle();

    signals:
        void sigSeqStarted();
        void sigSeqStopped();
        void gainChanged(float); // MasterSynthesizer
};

MuxSeqSig* muxseqsig_init();
MuxSeqSig* muxseqsig_get();
void muxseqsig_seq_emit_started();
void muxseqsig_seq_emit_stopped();
void muxseqsig_emit_gainChanged (float gain);

} // namespace Ms
#endif

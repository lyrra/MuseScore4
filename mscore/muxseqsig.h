
namespace Ms {

class MuxSeqSig : public QObject {
    public:
        ~MuxSeqSig();
        void emit_sigSeqStarted();
        void emit_sigSeqStopped();

    signals:
        void sigSeqStarted();
        void sigSeqStopped();
};

MuxSeqSig* muxseqsig_init();
MuxSeqSig* muxseqsig_get();
void muxseqsig_seq_emit_started();
void muxseqsig_seq_emit_stopped();
 
}

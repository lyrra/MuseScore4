#ifndef __MUXSEQTOOLS_H__
#define __MUXSEQTOOLS_H__

namespace Ms {

// tools
int muxseq_saveMp3(QIODevice* device,
                   Score* score,
                   bool useCurrentSynthesizerState, 
                   int sampleRate,
                   int bitRate,
                   bool useNativeDialogs);
} // namespace Ms
#endif

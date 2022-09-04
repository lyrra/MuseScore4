//FIX: USE_LAME isn't set (though is set in musescore.cpp)
//#ifdef USE_LAME
#include "exportmp3.h"
//#endif
#include "libmscore/score.h"
#include "libmscore/part.h"
#include "muxseq_client.h"
#include "msynthesizer.h"

namespace Ms {

//---------------------------------------------------------
//   canSaveMp3
//---------------------------------------------------------

bool muxseq_canSaveMp3() {
#ifndef USE_LAME
    return false;
#else
    MP3Exporter exporter;
    if (!exporter.loadLibrary(MP3Exporter::AskUser::NO)) {
        qDebug("Could not open MP3 encoding library!");
        return false;
    }

    if (!exporter.validLibraryLoaded()) {
        qDebug("Not a valid or supported MP3 encoding library!");
        return false;
    }
    return true;
#endif
}

int muxseq_saveMp3(QIODevice* device,
                   Score* score,
                   bool useCurrentSynthesizerState, 
                   int sampleRate,
                   int bitRate) {

    MP3Exporter exporter;
    if (!exporter.loadLibrary(MP3Exporter::AskUser::MAYBE)) {
        return -1;
    }
    if (!exporter.validLibraryLoaded()) {
        return -2;
    }
    // Retrieve preferences
    //      int highrate = 48000;
    //      int lowrate = 8000;
    //      int bitrate = 64;
    //      int brate = 128;
    //      int rmode = MODE_CBR;
    //      int vmode = ROUTINE_FAST;
    //      int cmode = CHANNEL_STEREO;
    int channels = 2;
    exporter.setBitrate(bitRate);
    int inSamples = exporter.initializeStream(channels, sampleRate);
    if (inSamples < 0) {
        return -3;
    }
    int bufferSize   = exporter.getOutBufferSize();
    uchar* bufferOut = new uchar[bufferSize];
// FIX: MOVE this to muxseq server-side
#if 0
    MasterSynthesizer* synth = muxseq_synthesizerFactory();
    synth->init();
    synth->setSampleRate(sampleRate);

    SynthesizerState state = useCurrentSynthesizerState ? muxseq_synti_get_synthesizerState() : score->synthesizerState();
    const bool setStateOk = synth->setState(state);
    if (!setStateOk || !synth->hasSoundFontsLoaded()) {
        synth->init(); // re-initialize master synthesizer with default settings
        synth->setSampleRate(sampleRate);
    }
    MScore::sampleRate = sampleRate;
    EventMap events;
    // In non-GUI mode current synthesizer settings won't
    // allow single note dynamics. See issue #289947.
    if (useCurrentSynthesizerState) {
          score->renderMidi(&events, muxseq_synti_get_synthesizerState());
          if (events.empty())
                return -99;
    } else {
        score->masterScore()->rebuildAndUpdateExpressive(synth->synthesizer("Fluid"));
        score->renderMidi(&events, score->synthesizerState());
        if (muxseq_get_synti()) {
            MasterSynthesizer* synti = muxseq_get_synti();
            score->masterScore()->rebuildAndUpdateExpressive(synti->synthesizer("Fluid"));
        }
        if (events.empty()) {
            return -99;
        }
    }
    //--------------------------------------------
    static const int FRAMES = 512;
    float bufferL[FRAMES];
    float bufferR[FRAMES];

    float  peak = 0.0;
    double gain = 1.0;
    EventMap::const_iterator endPos = events.cend();
    endPos--;
    const int et = (score->utick2utime(endPos->first) + 1) * MScore::sampleRate;
    const int maxEndTime = (score->utick2utime(endPos->first) + 3) * MScore::sampleRate;
    //progress.setRange(0, et);
    int ok = 0;

    for (int pass = 0; pass < 2; ++pass) {
        EventMap::const_iterator playPos;
        playPos = events.cbegin();
        synth->allSoundsOff(-1);

        //
        // init instruments
        //
        for (Part* part : score->parts()) {
              const InstrumentList* il = part->instruments();
              for (auto i = il->begin(); i!= il->end(); i++) {
                    for (const Channel* channel : i->second->channel()) {
                          const Channel* a = score->masterScore()->playbackChannel(channel);
                          for (MidiCoreEvent e : a->initList()) {
                                if (e.type() == ME_INVALID)
                                      continue;
                                e.setChannel(a->channel());
                                int syntiIdx= synth->index(score->masterScore()->midiMapping(a->channel())->articulation()->synti());
                                synth->play(e, syntiIdx);
                                }
                          }
                    }
              }

        int playTime = 0.0;

        for (;;) {
              unsigned frames = FRAMES;
              float max = 0;
              //
              // collect events for one segment
              //
              memset(bufferL, 0, sizeof(float) * FRAMES);
              memset(bufferR, 0, sizeof(float) * FRAMES);
              double endTime = playTime + frames;

              float* l = bufferL;
              float* r = bufferR;

              for (; playPos != events.cend(); ++playPos) {
                    double f = score->utick2utime(playPos->first) * MScore::sampleRate;
                    if (f >= endTime)
                          break;
                    int n = f - playTime;
                    if (n) {
#if (!defined (_MSCVER) && !defined (_MSC_VER))
                          float bu[n * 2];
                          memset(bu, 0, sizeof(float) * 2 * n);
#else
                          // MSVC does not support VLA. Replace with std::vector. If profiling determines that the
                          //    heap allocation is slow, an optimization might be used.
                          std::vector<float> vBu(n * 2, 0);   // Default initialized, memset() not required.
                          float* bu = vBu.data();
#endif

                          synth->process(n, bu);
                          float* sp = bu;
                          for (int i = 0; i < n; ++i) {
                                *l++ = *sp++;
                                *r++ = *sp++;
                                }
                          playTime  += n;
                          frames    -= n;
                          }
                    const NPlayEvent& e = playPos->second;
                    if (!(!e.velo() && e.discard()) && e.isChannelEvent()) {
                          int channelIdx = e.channel();
                          Channel* c = score->masterScore()->midiMapping(channelIdx)->articulation();
                          if (!c->mute()) {
                                synth->play(e, synth->index(c->synti()));
                                }
                          }
                    }
              if (frames) {
#if (!defined (_MSCVER) && !defined (_MSC_VER))
                    float bu[frames * 2];
                    memset(bu, 0, sizeof(float) * 2 * frames);
#else
                    // MSVC does not support VLA. Replace with std::vector. If profiling determines that the
                    //    heap allocation is slow, an optimization might be used.
                    std::vector<float> vBu(frames * 2, 0);   // Default initialized, memset() not required.
                    float* bu = vBu.data();
#endif
                    synth->process(frames, bu);
                    float* sp = bu;
                    for (unsigned i = 0; i < frames; ++i) {
                          *l++ = *sp++;
                          *r++ = *sp++;
                          }
                    playTime += frames;
                    }

              if (pass == 1) {
                    for (int i = 0; i < FRAMES; ++i) {
                          max = qMax(max, qAbs(bufferL[i]));
                          max = qMax(max, qAbs(bufferR[i]));
                          bufferL[i] *= gain;
                          bufferR[i] *= gain;
                          }
                    long bytes;
                    if (FRAMES < inSamples)
                          bytes = exporter.encodeRemainder(bufferL, bufferR,  FRAMES , bufferOut);
                    else
                          bytes = exporter.encodeBuffer(bufferL, bufferR, bufferOut);
                    if (bytes < 0) {
                          ok = -4;
                          break;
                    } else {
                        device->write((char*)bufferOut, bytes);
                    }
              } else {
                    for (int i = 0; i < FRAMES; ++i) {
                          max = qMax(max, qAbs(bufferL[i]));
                          max = qMax(max, qAbs(bufferR[i]));
                          peak = qMax(peak, qAbs(bufferL[i]));
                          peak = qMax(peak, qAbs(bufferR[i]));
                          }
                    }
              playTime = endTime;
              //if (!MScore::noGui) {
              //      if (progress.wasCanceled())
              //            break;
              //      progress.setValue((pass * et + playTime) / 2);
              //      qApp->processEvents();
              //      }
              if (playTime >= et)
                    synth->allNotesOff(-1);
              // create sound until the sound decays
              if (playTime >= et && max * peak < 0.000001)
                    break;
              // hard limit
              if (playTime > maxEndTime)
                    break;
              }
        //if (progress.wasCanceled())
        //      break;
        if (pass == 0 && peak == 0.0) {
              qDebug("song is empty");
              break;
              }
        gain = 0.99 / peak;
    }

    long bytes = exporter.finishStream(bufferOut);
    if (bytes > 0L)
          device->write((char*)bufferOut, bytes);
    //wasCanceled = progress.wasCanceled();
    delete synth;
#endif
    delete[] bufferOut;
    return 0;
}

} // namespace Ms

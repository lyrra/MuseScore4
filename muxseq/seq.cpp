//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "config.h"
#include <chrono>
#include <thread>
#include "seq.h"
#include "event.h"
#include "muxlib.h"
#include "muxseq.h"
#include "muxseqsig.h"

#include "msynthesizer.h"

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

#ifdef USE_PORTMIDI
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
  #include "portmidi/porttime/porttime.h"
#else
  #include <porttime.h>
#endif
#endif

namespace Ms {
void* muxseq_mscore_query (MuxseqMsgType type, int i);
int muxseq_mscore_tell (MuxseqMsgType type, int i);

double g_sampleRate = 48000; //FIX: poll muxaudio
int g_driver_running = 0;
int g_mscore_division = 1; // FIX: need to set this at start


uint64_t g_utime;
uint64_t g_utick;

void muxseq_stop_threads();
extern int g_muxseq_audio_process_run;
static Transport jack_transport;

Seq* seq = nullptr; // FIX: rename to g_seq
Seq* g_seq = nullptr;

static const int guiRefresh   = 10;       // Hz
static const int peakHoldTime = 1400;     // msec
static const int peakHold     = (peakHoldTime * guiRefresh) / 1000;
static OggVorbis_File vf;

static constexpr int minUtickBufferSize = 480 * 4 * 10; // about 10 measures of 4/4 time signature

#if 0 // yet(?) unused
static const int AUDIO_BUFFER_SIZE = 1024 * 512;  // 2 MB
#endif

/*
 * Preferences
 * FIX: move this to muxlib
 */
enum PREF {
    PREF_IO_JACK_USEJACKTRANSPORT = 0
};

struct musescore_prefs_t {
    bool pref_PREF_IO_JACK_USEJACKTRANSPORT;
    bool (* getBool)(PREF);
} preferences;


bool getBool(PREF p) {
    switch (p) {
        case PREF_IO_JACK_USEJACKTRANSPORT: return preferences.pref_PREF_IO_JACK_USEJACKTRANSPORT;
        default: qFatal("No such preference: %i", p);
    }
}

void init_preferences ()
{
    preferences.pref_PREF_IO_JACK_USEJACKTRANSPORT = false;
    preferences.getBool = &getBool;
}

/*
 *
 *
 */

int seq_create(int sampleRate) {
    LD("seq_create -- create sequencer");
    g_seq = seq = new Seq();
    g_seq->setMasterSynthesizer(nullptr);
    init_preferences();
    return 0;
}


//---------------------------------------------------------
//   VorbisData
//---------------------------------------------------------

struct VorbisData {
      int pos;          // current position in audio->data()
      QByteArray data;
      };

static VorbisData vorbisData;

static size_t ovRead(void* ptr, size_t size, size_t nmemb, void* datasource);
static int ovSeek(void* datasource, ogg_int64_t offset, int whence);
static long ovTell(void* datasource);

static ov_callbacks ovCallbacks = {
      ovRead, ovSeek, 0, ovTell
      };

//---------------------------------------------------------
//   ovRead
//---------------------------------------------------------

static size_t ovRead(void* ptr, size_t size, size_t nmemb, void* datasource)
      {
      VorbisData* vd = (VorbisData*)datasource;
      size_t n = size * nmemb;
      if (vd->data.size() < int(vd->pos + n))
            n = vd->data.size() - vd->pos;
      if (n) {
            const char* src = vd->data.data() + vd->pos;
            memcpy(ptr, src, n);
            vd->pos += int(n);
            }
      return n;
      }

//---------------------------------------------------------
//   ovSeek
//---------------------------------------------------------

static int ovSeek(void* datasource, ogg_int64_t offset, int whence)
      {
      VorbisData* vd = (VorbisData*)datasource;
      switch(whence) {
            case SEEK_SET:
                  vd->pos = offset;
                  break;
            case SEEK_CUR:
                  vd->pos += offset;
                  break;
            case SEEK_END:
                  vd->pos = vd->data.size() - offset;
                  break;
            }
      return 0;
      }

//---------------------------------------------------------
//   ovTell
//---------------------------------------------------------

static long ovTell(void* datasource)
      {
      VorbisData* vd = (VorbisData*)datasource;
      return vd->pos;
      }

//---------------------------------------------------------
//   Seq
//---------------------------------------------------------

Seq::Seq()
   : midi(nullptr)
      {
      running         = false;
      playlistChanged = false;
      //cs              = 0;
      //cv              = 0;
      tackRemain        = 0;
      tickRemain        = 0;
      maxMidiOutPort  = 0;

      endUTick  = 0;
      state    = Transport::STOP;
      oggInit  = false;
      playPos  = events.cbegin();
      playFrame  = 0;
      metronomeVolume = 0.3;
      useJackTransportSavedFlag = false;

      inCountIn         = false;
      countInPlayPos    = countInEvents.cbegin();
      countInPlayFrame  = 0;

      meterValue[0]     = 0.0;
      meterValue[1]     = 0.0;
      meterPeakValue[0] = 0.0;
      meterPeakValue[1] = 0.0;
      peakTimer[0]       = 0;
      peakTimer[1]       = 0;

      heartBeatTimer = new QTimer(this);
      connect(heartBeatTimer, SIGNAL(timeout()), this, SLOT(heartBeatTimeout()));

      noteTimer = new QTimer(this);
      noteTimer->setSingleShot(true);
      connect(noteTimer, SIGNAL(timeout()), this, SLOT(stopNotes()));
      noteTimer->stop();

      prevTimeSig.setNumerator(0);
      prevTempo = 0;
      connect(this, SIGNAL(timeSigChanged()),this,SLOT(handleTimeSigTempoChanged()));
      connect(this, SIGNAL(tempoChanged()),this,SLOT(handleTimeSigTempoChanged()));

      initialMillisecondTimestampWithLatency = 0;
      }

//---------------------------------------------------------
//   Seq
//---------------------------------------------------------

Seq::~Seq()
      {
      qDebug("!!!! qseq destructor !!!!");
      }

//---------------------------------------------------------
//   setScoreView
//---------------------------------------------------------

#if 0 //FIX: reenable and move this into musescore (or mscore/seq side). Unmarked code has moved to muxseq_client.cpp:muxseq_seq_set_scoreview
//FIX: perhaps update the score-instrument map here
void Seq::setScoreView(ScoreView* v)
      {
      if (oggInit) {
            ov_clear(&vf);
            oggInit = false;
            }
      if (cv !=v && cs) {
            unmarkNotes();
            stopWait();
            }
      //cv = v;
      //if (cs)
            disconnect(cs, SIGNAL(playlistChanged()), this, SLOT(setPlaylistChanged()));
      //cs = cv ? cv->score()->masterScore() : 0;
//      midi = MidiRenderer(cs);   // see muxseq_client.cpp:maybe_update_midiRenderer()
//      midi.setMinChunkSize(10);

      if (!heartBeatTimer->isActive())
            heartBeatTimer->start(20);    // msec

      playlistChanged = true;
      //_synti->reset();
      //if (cs) {
            //initInstruments();
            connect(cs, SIGNAL(playlistChanged()), this, SLOT(setPlaylistChanged()));
            //}
      }
#endif

//---------------------------------------------------------
//   Seq::CachedPreferences::update
//---------------------------------------------------------

#if 0
void Seq::CachedPreferences::update()
      {
#ifdef USE_PORTMIDI
      portMidiOutputLatencyMilliseconds = preferences.getInt(PREF_IO_PORTMIDI_OUTPUTLATENCYMILLISECONDS);
#endif
      jackTimeBaseMaster = preferences.getBool(PREF_IO_JACK_TIMEBASEMASTER);
      useJackTransport = preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT);
      useJackMidi = preferences.getBool(PREF_IO_JACK_USEJACKMIDI);
      useJackAudio = preferences.getBool(PREF_IO_JACK_USEJACKAUDIO);
      useAlsaAudio = preferences.getBool(PREF_IO_ALSA_USEALSAAUDIO);
#ifdef USE_PORTMIDI
      usePortAudio = preferences.getBool(PREF_IO_PORTAUDIO_USEPORTAUDIO);
#endif
      usePulseAudio = preferences.getBool(PREF_IO_PULSEAUDIO_USEPULSEAUDIO);
      }
#endif

//---------------------------------------------------------
//   startTransport
//---------------------------------------------------------

void Seq::startTransport()
      {
      //FIX: cachedPrefs.update();
      qDebug("-- Seq::startTransport");
      muxseq_msg_to_audio(MsgTypeTransportStart, 0);
      }

//---------------------------------------------------------
//   stopTransport
//---------------------------------------------------------

void Seq::stopTransport()
      {
      muxseq_msg_to_audio(MsgTypeTransportStop, 0);
      }

//---------------------------------------------------------
//   init
//    return false on error
//---------------------------------------------------------

bool Seq::init(bool hotPlug)
      {
      if (hotPlug) {
            muxseq_msg_to_audio(MsgTypeAudioInit, hotPlug);
      }
      LD("Seq::init g_muxseq_audio_process_run = 1\n");
      g_muxseq_audio_process_run = 1; //FIX: move this into muxseq (by a function call)
      muxseq_msg_to_audio(MsgTypeAudioStart, hotPlug);
      LD("Seq::init waiting for muxaudio to start g_driver\n");
      while (! g_driver_running /* g_ctrl_audio_running */) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
            //FIX: if (g_ctrl_audio_error) {
            //      running = false;
            //      return false;
            //      }
      }
      LD("Seq::init muxaudio has started for g_driver\n");
      //FIX: cachedPrefs.update();
      running = true;
      return true;
}

//---------------------------------------------------------
//   exit
//---------------------------------------------------------

void Seq::exit()
      {
      muxseq_stop_threads();
      muxseq_msg_to_audio(MsgTypeAudioStop, 0);
      }

//---------------------------------------------------------
//   rewindStart
//---------------------------------------------------------

void Seq::rewindStart()
      {
      seek(0);
      }

//---------------------------------------------------------
//   loopStart
//---------------------------------------------------------

void Seq::loopStart()
      {
      start();
//      qDebug("LoopStart. playPos = %d", playPos);
      }

//---------------------------------------------------------
//   canStart
//    return true if sequencer can be started
//---------------------------------------------------------

bool Seq::canStart()
      {
      LD4("Seq::canStart");
      if (! g_driver_running)
            return false;
      collectEvents(getPlayStartUtick());
      LD2("Seq::canStart empty:%i endUTick=%i", events.empty(), endUTick);
      return (!events.empty() && endUTick != 0);
      }

//---------------------------------------------------------
//   start
//    called from gui thread
//---------------------------------------------------------

void Seq::start()
      {
      LD4("Seq::start !!!!! time to play some notes ----");

      if (! g_driver_running) {
            qDebug("seq cant start: driver is not running!");
            return;
            }

//      mscore->moveControlCursor(); //FIX: enable move to muxseq

      allowBackgroundRendering = true;
      collectEvents(getPlayStartUtick());
      LD6("Seq::start done with collectEvents");
      if (true /* FIX: cs->playMode() == PlayMode::AUDIO */) {
            if (!oggInit) {
                  vorbisData.pos  = 0;
                  //FIX: vorbisData.data = cs->audio()->data();
                  //FIX: send to ogg-plugin
                  //int n = ov_open_callbacks(&vorbisData, &vf, 0, 0, ovCallbacks);
                  //if (n < 0) {
                  //      qDebug("ogg open failed: %d", n);
                  //      }
                  oggInit = true;
                  }
            }

      if (!preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) ||
          (preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) && state == Transport::STOP)) {
            LD6("Seq::start state=%i, Transport::STOP=%i", state, Transport::STOP);
            }
      if (state == Transport::STOP) {
            seek(getPlayStartUtick());
            }

      // FIX: send to mscore
      //if (preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) && mscore->countIn() && state == Transport::STOP) {
            // Ready to start playing count in, switching to fake transport
            // to prevent playing in other applications with our ticks simultaneously
      //      useJackTransportSavedFlag    = true;
      //      preferences.setPreference(PREF_IO_JACK_USEJACKTRANSPORT, false);
      //      }
      LD6("Seq::start calling startTransport");
      startTransport();
      }

//---------------------------------------------------------
//   stop
//    called from gui thread
//---------------------------------------------------------

void Seq::stop()
      {
      qDebug("---- Seq::stop ----------------------");
      const bool seqStopped = (state == Transport::STOP);
      const bool driverStopped = !g_driver_running || jack_transport == Transport::STOP;
      if (seqStopped && driverStopped)
            return;

      allowBackgroundRendering = false;
      if (oggInit) {
            //ov_clear(&vf); //FIX: send to oggplugin
            oggInit = false;
            }
      if (!g_driver_running)
            return;
      //FIX: if (!preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) || (preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT) && jack_transport == Transport::PLAY))
      if (jack_transport == Transport::PLAY)
            stopTransport();
      //FIX: send to mscore
      //if (cv)
      //      cv->setCursorOn(false);
      if (midiRenderFuture.isRunning())
            midiRenderFuture.waitForFinished();
      //FIX: send to mscore
      //if (cs) {
      //      cs->setUpdateAll();
      //      cs->update();
      //      }
      }

//---------------------------------------------------------
//   stopWait
//---------------------------------------------------------

void Seq::stopWait()
      {
      stop();
      QWaitCondition sleep;
      int idx = 0;
      while (state != Transport::STOP) {
            qDebug("State %d", (int)state);
            mutex.lock();
            sleep.wait(&mutex, 100);
            mutex.unlock();
            ++idx;
            Q_ASSERT(idx <= 10);
            }
      }


//---------------------------------------------------------
//   unmarkNotes
//---------------------------------------------------------

void Seq::unmarkNotes()
      {
#if 0 //FIX: Note unknown type
      foreach(const Note* n, markedNotes) {
            n->setMark(false);
            //FIX: send to mscore
            //cs->addRefresh(n->canvasBoundingRect());
            }
      markedNotes.clear();
#endif
      //FIX: send markednotes to mscore
      //PianoTools* piano = mscore->pianoTools();
      //if (piano && piano->isVisible())
      //      piano->setPlaybackNotes(markedNotes);
      }

//---------------------------------------------------------
//   guiStop
//---------------------------------------------------------

void Seq::guiStop()
      {
      LD("Seq::guiStop");
//FIX: send to mscore
//      QAction* a = getAction("play");
//      a->setChecked(false);

      unmarkNotes();
//      if (!cs)
//            return;

      muxseq_mscore_tell(MsgTypeSeqStopped, playFrame);
      }

//---------------------------------------------------------
//   seqSignal
//    sequencer message to GUI
//    execution environment: gui thread
//---------------------------------------------------------

void Seq::seqMessage(int msg, int arg)
      {
      switch(msg) {
            case '5': {
                  // Update the screen after seeking from the realtime thread
                  muxseq_mscore_tell(MsgTypeSeqSeek, arg);
#if 0 //FIX: handle in muxseq_client
                  Segment* seg = cs->tick2segment(Fraction::fromTicks(arg));
                  if (seg)
                        mscore->currentScoreView()->moveCursor(seg->tick());
                  cs->setPlayPos(Fraction::fromTicks(arg));
                  cs->update();
#endif
                  break;
                  }
            case '4':   // Restart the playback at the end of the score
                  loopStart();
                  break;
            case '3':   // Loop restart while playing
                  //FIX: send to mscore seek(cs->repeatList().tick2utick(cs->loopInTick().ticks()));
                  break;
            case '2':
                  //FIX: muxseq mscore-zmq side of ringbuffer, should take MsgTypeSeqStopped messages, send to mscore, and do following code:
                  guiStop();
//                  heartBeatTimer->stop();
                  if (g_driver_running /* FIX: && mscore->getSynthControl() */) {
                        meterValue[0]     = .0f;
                        meterValue[1]     = .0f;
                        meterPeakValue[0] = .0f;
                        meterPeakValue[1] = .0f;
                        peakTimer[0]       = 0;
                        peakTimer[1]       = 0;
                        //FIX: mscore->getSynthControl()->setMeter(0.0, 0.0, 0.0, 0.0);
                        }
                  seek(0);
                  break;
            case '0':         // STOP
                  guiStop();
//                  heartBeatTimer->stop();
                  if (g_driver_running /* FIX: && mscore->getSynthControl() */) {
                        meterValue[0]     = .0f;
                        meterValue[1]     = .0f;
                        meterPeakValue[0] = .0f;
                        meterPeakValue[1] = .0f;
                        peakTimer[0]       = 0;
                        peakTimer[1]       = 0;
                        //FIX: mscore->getSynthControl()->setMeter(0.0, 0.0, 0.0, 0.0);
                        }
                  break;

            case '1':         // PLAY
                    LD("Seq::seqMessage muxseq_mscore_tell MsgTypeSeqStarted");
                    muxseq_mscore_tell(MsgTypeSeqStarted, 0);
//                  heartBeatTimer->start(1000/guiRefresh);
                  break;

            default:
                  LD("Seq::seqMessage unknown seq msg %d", msg);
                  break;
            }
      }

//---------------------------------------------------------
//   playEvent
//    send one event to the synthesizer
//---------------------------------------------------------

void Seq::playEvent(const NPlayEvent& event, unsigned framePos)
      {
      int type = event.type();
      if (type == ME_NOTEON) {
            if (!event.isMuted()) {
                  if (event.discard()) { // ignore noteoff but restrike noteon
                        if (event.velo() > 0)
                              putEvent(NPlayEvent(ME_NOTEON, event.channel(), event.pitch(), 0) ,framePos);
                        else
                              return;
                        }
                  putEvent(event, framePos);
                  //FIX: move this code to musescore
                  //PianoTools* piano = mscore->pianoTools();
                  //if (piano && piano->isVisible()) {
                  //      if (event.velo() > 0)
                  //            piano->pressPlaybackPitch(event.pitch());
                  //      else // Note-offs are synthesized as ME_NOTEON with 0 velocity
                  //            piano->releasePlaybackPitch(event.pitch());
                  //      }
                  }
            }
      else if (type == ME_CONTROLLER || type == ME_PITCHBEND || type == ME_AFTERTOUCH || type == ME_POLYAFTER)
            putEvent(event, framePos);
      }

//---------------------------------------------------------
//   recomputeMaxMidiOutPort
//   Computes the maximum number of midi out ports
//   in all opened scores
//---------------------------------------------------------

void Seq::recomputeMaxMidiOutPort()
      {
      //if (!(preferences.getBool(PREF_IO_JACK_USEJACKMIDI) || preferences.getBool(PREF_IO_ALSA_USEALSAAUDIO)))
      //      return;
      //FIX: send to mscore query
#if 0
      int max = 0;
      for (Score * s : MuseScoreCore::mscoreCore->scores()) {
            if (s->masterScore()->midiPortCount() > max)
                  max = s->masterScore()->midiPortCount();
            }
      maxMidiOutPort = max;
#endif
      }



//---------------------------------------------------------
//   processMessages
//   from gui to process thread
//---------------------------------------------------------

void Seq::processMessages()
      {
#if 0 //FIX: remove this message-queue (mscore uses zmq over network instead)
      for (;;) {
            if (toSeq.empty())
                  break;
            SeqMsg msg = toSeq.dequeue();
            switch(msg.id) {
                  case SeqMsgId::TEMPO_CHANGE:
                        {
                        if (!cs)
                              continue;
                        if (playFrame != 0) {
                              int utick = cs->utime2utick(qreal(playFrame) / qreal(g_sampleRate));
                              cs->tempomap()->setRelTempo(msg.realVal);
                              if (cachedPrefs.jackTimeBaseMaster && cachedPrefs.useJackTransport)
                                    msgToAudioSeekTransport(utick); // + 2 * cs->utime2utick(qreal((_driver->bufferSize()) + 1) / qreal(g_sampleRate))
                              }
                        else
                              cs->tempomap()->setRelTempo(msg.realVal);
                        cs->masterScore()->updateRepeatListTempo();
                        prevTempo = curTempo();
                        emit tempoChanged();
                        }
                        break;
                  case SeqMsgId::PLAY:
                        putEvent(msg.event);
                        break;
                  case SeqMsgId::SEEK:
                        setPos(msg.intVal);
                        break;
                  case SeqMsgId::ALL_NOTE_OFF:
                        {
                        _synti->allNotesOff(msg.intVal);
                        PianoTools* piano = mscore->pianoTools();
                        // PianoTools here returns the playback pitches by-value
                        // to ensure no discrepancy while erasing items from
                        // the _pressedPlaybackPitches container
                        for (int pressedPitch : piano->pressedPlaybackPitches())
                              piano->releasePlaybackPitch(pressedPitch);
                        }
                        break;
                  default:
                        break;
                  }
            }
#endif
      }

//---------------------------------------------------------
//   metronome
//---------------------------------------------------------

void Seq::metronome(unsigned n, float* p, bool force)
      {
#if 0
      if (!mscore->metronome() && !force) {
            tickRemain = 0;
            tackRemain = 0;
            return;
            }
      if (tickRemain) {
            tackRemain = 0;
            int idx = tickLength - tickRemain;
            int nn = n < tickRemain ? n : tickRemain;
            for (int i = 0; i < nn; ++i) {
                  qreal v = tick[idx] * tickVolume * metronomeVolume;
                  *p++ += v;
                  *p++ += v;
                  ++idx;
                  }
            tickRemain -= nn;
            }
      if (tackRemain) {
            int idx = tackLength - tackRemain;
            int nn = n < tackRemain ? n : tackRemain;
            for (int i = 0; i < nn; ++i) {
                  qreal v = tack[idx] * tackVolume * metronomeVolume;
                  *p++ += v;
                  *p++ += v;
                  ++idx;
                  }
            tackRemain -= nn;
            }
#endif
      }

//---------------------------------------------------------
//   addCountInClicks
//---------------------------------------------------------

void Seq::addCountInClicks()
      {
#if 0 // FIX: send to mscore, need position and score information
      const Fraction plPos = cs->playPos();
      Measure*    m        = cs->tick2measure(plPos);
      Fraction   msrTick   = m->tick();
      qreal tempo          = cs->tempomap()->tempo(msrTick.ticks());
      TimeSigFrac timeSig  = cs->sigmap()->timesig(m->tick()).nominal();

      const int clickTicks = timeSig.isBeatedCompound(tempo) ? timeSig.beatTicks() : timeSig.dUnitTicks();

      // add at least one full measure of just clicks.
      Fraction endTick = Fraction::fromTicks(timeSig.ticksPerMeasure());

      // add extra clicks if...
      endTick += plPos - msrTick;   // ...not starting playback at beginning of measure

      if (m->isAnacrusis())         // ...measure is incomplete (anacrusis)
            endTick += Fraction::fromTicks(timeSig.ticksPerMeasure()) - m->ticks();

      for (int t = 0; t < endTick.ticks(); t += clickTicks) {
            const int rtick = t % timeSig.ticksPerMeasure();
            countInEvents.insert(std::pair<int,NPlayEvent>(t, NPlayEvent(timeSig.rtick2beatType(rtick))));
            }

      NPlayEvent event;
      event.setType(ME_INVALID);
      event.setPitch(0);
      countInEvents.insert( std::pair<int,NPlayEvent>(endTick.ticks(), event));
      // initialize play parameters to count-in events
      countInPlayPos  = countInEvents.cbegin();
      countInPlayFrame = 0;
#endif
      }

/*
 *
 */


int mux_is_score_open () {
    //FIX: return seq->score() ? 1 : 0;
    return 1;
}

void mux_send_event_to_gui(struct SparseEvent se)
{
   /* //FIX: include muxlib which has event.cpp
    Event e;
    e.setType(se.type);
    e.setChannel(se.channel);
    e.setPitch(se.pitch);
    e.setVelo(se.velo);
    e.setController(se.cont);
    e.setValue(se.val);
    seq->eventToGui(e);
    */
}

static unsigned int jack_position_frame;
static unsigned int jack_position_valid;
static unsigned int jack_position_beats_per_minute;
static unsigned int jack_position_BBT;

void mux_set_jack_position(struct JackTransportPosition jackTransportPosition)
{
    jack_transport = static_cast<Transport>(jackTransportPosition.state);
    jack_position_frame = jackTransportPosition.frame;
    jack_position_valid = jackTransportPosition.valid;
    jack_position_beats_per_minute = jackTransportPosition.beats_per_minute;
    jack_position_BBT = jackTransportPosition.bbt;
    //LD("mux_set_jack_position jack_transport state=%i frame=%i\n", (int)jack_transport, jack_position_frame);
}

//---------------------------------------------------------
//   checkTransportSeek
//   The opposite of Timebase master:
//   check JACK Transport for a new position or tempo.
//---------------------------------------------------------

static void checkTransportSeek(int cur_frame, int nframes, bool inCountIn)
      {
      //qDebug("--- Seq::checkTransportSeek cur_frame=%i frames=%i", cur_frame, nframes);
//      if (!seq || !seq->score() || inCountIn)
//            return;

#if 0
      if (preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT)) {
            if (mscore->playPanelInterface() && mscore->playPanelInterface()->isSpeedSliderPressed())
                   return;
            int cur_utick = seq->score()->utime2utick((qreal)cur_frame / g_sampleRate);
            int utick     = seq->score()->utime2utick((qreal)jack_position_frame / g_sampleRate);

            // Conversion is not precise, should check frames and uticks
            if (labs((long int)cur_frame - (long int)jack_position_frame)>nframes + 1 && abs(utick - cur_utick)> seq->score()->utime2utick((qreal)nframes / g_sampleRate) + 1) {
                  qDebug()<<"JACK Transport position changed, cur_frame: "<<cur_frame<<",pos.frame: "<<jack_position_frame<<", frame diff: "<<labs((long int)cur_frame - (long int)jack_position_frame)<<"cur utick:"<<cur_utick<<",seek to utick: "<<utick<<", tick diff: "<<abs(utick - cur_utick);
                  seq->seekRT(utick);
                  }
            }
#endif

      // Tempo
      // FIX: if (!preferences.getBool(PREF_IO_JACK_TIMEBASEMASTER)  && (jack_position_valid & jack_position_BBT)) {
      if (jack_position_valid & jack_position_BBT) {
            //FIX: if (!seq->score()->tempomap())
            //      return;

#if 0 //FIX: score and mscore unknown
            if (int(jack_position_beats_per_minute) != int(60 * seq->curTempo() * seq->score()->tempomap()->relTempo())) {
                  //FIX: qDebug()<<"JACK Transport tempo changed! JACK bpm: "<<(int)jack_position_beats_per_minute<<", current bpm: "<<int(60 * seq->curTempo() * seq->score()->tempomap()->relTempo());
                  if (60 * seq->curTempo() == 0.0)
                        return;
                  qreal newRelTempo = jack_position_beats_per_minute / (60* seq->curTempo());
                  seq->setRelTempo(newRelTempo);
                  // Update UI
                  if (mscore->getPlayPanel()) {
                        mscore->playPanelInterface()->setSpeed(newRelTempo);
                        mscore->playPanelInterface()->setTempo(seq->curTempo() * newRelTempo);
                        }
                  }
#endif
            }
      }

//-------------------------------------------------------------------
//   process
//    This function is called in a realtime context. This
//    means that no blocking operations are allowed which
//    includes memory allocation. The usual thread synchronisation
//    methods like semaphores can also not be used.
//-------------------------------------------------------------------
void Seq::process(struct MuxaudioBuffer *mabuf)
      {
      float *buffer = mabuf->buf;
      unsigned int framesPerPeriod = MUX_CHUNK_NUMFLOATS / 2; // FIX: 2 = MUX_CHAN is fixed
      unsigned framesRemain = framesPerPeriod; // the number of frames remaining to be processed by this call to Seq::process
      //FIX: store number of generated frames in the mabuf
      //Transport driverState = seq->isPlaying() ? jack_transport : Transport::STOP;
      Transport driverState = jack_transport;
      // Checking for the reposition from JACK Transport
      checkTransportSeek(playFrame, framesRemain, inCountIn);

      LD4("Seq::process frames=%i driverState=%i state=%i", framesPerPeriod, (int) driverState, (int) state);
      if (driverState != state) {
            // Got a message from JACK Transport panel: Play
            if (state == Transport::STOP && driverState == Transport::PLAY) {
                  if ((cachedPrefs.useJackMidi || cachedPrefs.useJackAudio)
                       //FIX: && !getAction("play")->isChecked()
                     ) {
                        // Do not play while editing elements
                        if (/* FIX: mscore->state() != STATE_NORMAL || */ !isRunning() || !canStart())
                              return;
                        //FIX: getAction("play")->setChecked(true);
                        //FIX: getAction("play")->triggered(true);

                        // If we just launch MuseScore and press "Play" on JACK Transport with time 0:00
                        // MuseScore doesn't seek to 0 and guiPos is uninitialized, so let's make it manually
                        if (cachedPrefs.useJackTransport && getCurTick() == 0)
                              seekRT(0);

                        // Switching to fake transport while playing count in
                        // to prevent playing in other applications with our ticks simultaneously
                        if (cachedPrefs.useJackTransport /* FIX: && mscore->countIn() */) {
                              // Stopping real JACK Transport
                              stopTransport();
                              // Starting fake transport
                              useJackTransportSavedFlag = false; //FIX: preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT);
                              //FIX: preferences.setPreference(PREF_IO_JACK_USEJACKTRANSPORT, false);
                              startTransport();
                              }
                        }
                  // Initializing instruments every time we start playback.
                  // External synth can have wrong values, for example
                  // if we switch between scores
                  //initInstruments(true, -1); //FIX: disabled for-now, need score-instrument map
                  // Need to change state after calling collectEvents()
                  state = Transport::PLAY;
//FIX:                  if (mscore->countIn() && cs->playMode() == PlayMode::SYNTHESIZER) {
//                        countInEvents.clear();
//                        inCountIn = true;
//                        }
                  muxseq_mscore_tell(MsgTypeSeqStarted, 0);
                  }
            // Got a message from JACK Transport panel: Stop
            else if (state == Transport::PLAY && driverState == Transport::STOP) {
                  state = Transport::STOP;
                  // Muting all notes
                  stopNotes(-1, true);
                  //initInstruments(true, -1); //FIX: disabled for-now, need score-instrument map
                  if (playPos == eventsEnd) {
#if 0 //FIX mscore not known
                        if (mscore->loop()) {
                              //qDebug("Seq.cpp - Process - Loop whole score. playPos = %d, cs->pos() = %d", playPos->first, cs->pos().ticks());
                              //FIX: emit toGui('4');
                              return;
                              }
                        else {
                              LD("########## STATE STOPPING-PLAY ############")
                              // FIX: see FIX-note in case '2' in seqMessage
                              muxseq_mscore_tell(MsgTypeSeqStopped, playFrame);
                              }
#endif
                        }
                  else {
                     // seqMessage case 0:
                     muxseq_mscore_tell(MsgTypeSeqStopped, playFrame);
                     }
                  }
            else if (state != driverState) {
                  LD6("Seq::process state transition %d -> %d ?",
                      (int)state, (int)driverState);
                  }
            }

      //FIX: instead count how much buffer is left to zero
      memset(buffer, 0, sizeof(float) * framesPerPeriod * 2); // assume two channels
      float* pBuffer = buffer;

      processMessages();

      LD6("Seq::process state=%i (play=%i)", (int) state, (int) Transport::PLAY);
      if (state == Transport::PLAY) {
            // FIX: if (!cs)
            //      return;

            // if currently in count-in, these pointers will reference data in the count-in
            EventMap::const_iterator* pPlayPos   = &playPos;
            EventMap::const_iterator  pEventsEnd = eventsEnd;
            int*                      pPlayFrame = &playFrame;
            if (inCountIn) {
                  qDebug("Seq::process #### WARNING inCountIn active! NOT IMPL ####");
                  if (countInEvents.size() == 0)
                        addCountInClicks();
                  pEventsEnd = countInEvents.cend();
                  pPlayPos   = &countInPlayPos;
                  pPlayFrame = &countInPlayFrame;
                  }
            //
            // play events for one segment
            //
            unsigned framePos = 0; // frame currently being processed relative to the first frame of this call to Seq::process
            int periodEndFrame = *pPlayFrame + framesPerPeriod; // the ending frame (relative to start of playback) of the period being processed by this call to Seq::process
            int scoreEndUTick = 0; //cs->repeatList().tick2utick(cs->lastMeasure()->endTick().ticks());
            LD6("Seq::process -- loop events %i to %i", (*pPlayPos)->first, pEventsEnd->first);
            while (*pPlayPos != pEventsEnd) {
                  int playPosUTick = (*pPlayPos)->first;
                  int n; // current frame (relative to start of playback) that is being synthesized
                  {
                      const NPlayEvent& event = (*pPlayPos)->second;
                      LD6("Seq::process -- playPosUTick=%i pitch=%i channel=%i", playPosUTick, event.pitch(), event.channel());
                  }
                  g_utick = playPosUTick;
                  if (inCountIn) {
                        LD8("Seq::process -- #### WARNING inCountIn, NOT IMPL ####");
#if 0
                        qreal beatsPerSecond = curTempo() * cs->tempomap()->relTempo(); // relTempo needed here to ensure that bps changes as we slide the tempo bar
                        qreal ticksPerSecond = beatsPerSecond * g_mscore_division;
                        qreal playPosSeconds = playPosUTick / ticksPerSecond;
                        int playPosFrame = playPosSeconds * g_sampleRate;
                        if (playPosFrame >= periodEndFrame)
                              break;
                        n = playPosFrame - *pPlayFrame;
                        if (n < 0) {
                              LD6("Count-in: playPosUTick %d: n = %d - %d", playPosUTick, playPosFrame, *pPlayFrame);
                              n = 0;
                              }
#endif
                        }
                  else {
                        const NPlayEvent& event = (*pPlayPos)->second;
                        qreal playPosSeconds = event.playPosSeconds; // FIX: cs->utick2utime(playPosUTick);
                        int playPosFrame = playPosSeconds * g_sampleRate;
                        LD6("Seq::process -- loop events [%i,%i] playPosFrame=%i periodEndFrame=%i pPlayFrame=%i playPosSeconds=%f g_sampleRate=%f", (*pPlayPos)->first, pEventsEnd->first, playPosFrame, periodEndFrame, *pPlayFrame, playPosSeconds, g_sampleRate);
                        if (playPosFrame >= periodEndFrame)
                              break;
                        n = playPosFrame - *pPlayFrame;
                        if (n > framesRemain) {
                              LD6("Seq::process WARNING requested %i frames, only %i frames in buffer", n, framesRemain);
                              }
                        if (n < 0) {
                              qDebug("%d:  %d - %d", playPosUTick, playPosFrame, *pPlayFrame);
                              n = 0;
                              }
#if 0
                        if (mscore->loop()) {
                              int loopOutUTick = cs->repeatList().tick2utick(cs->loopOutTick().ticks());
                              if (loopOutUTick < scoreEndUTick) {
                                    qreal framesPerPeriodInTime = static_cast<qreal>(framesPerPeriod) / g_sampleRate;
                                    int framesPerPeriodInTicks = cs->utime2utick(framesPerPeriodInTime);
                                    // Also make sure we are inside the loop
                                    if (playPosUTick >= loopOutUTick - 2 * framesPerPeriodInTicks || cs->repeatList().utick2tick(playPosUTick) < cs->loopInTick().ticks()) {
                                          LD8("Seq::Process -- playPosUTick=%d cs->loopInTick().ticks()=%d cs->loopOutTick().ticks()=%d getCurTick()=%d loopOutUTick=%d playFrame=%d",
                                                  playPosUTick, cs->loopInTick().ticks(), cs->loopOutTick().ticks(), getCurTick(), loopOutUTick, *pPlayFrame);
                                          if (cachedPrefs.useJackTransport) {
                                                int loopInUTick = cs->repeatList().tick2utick(cs->loopInTick().ticks());
                                                msgToAudioSeekTransport(loopInUTick);
                                                if (loopInUTick != 0) {
                                                      int seekto = loopInUTick; // - 2 * cs->utime2utick((qreal)_driver->bufferSize() / g_sampleRate);
                                                      seekRT((seekto > 0) ? seekto : 0 );
                                                      }
                                                }
                                          else {
                                                emit toGui('3'); // calls loopStart()
                                                }
                                          // Exit this function to avoid segmentation fault in Scoreview
                                          return;
                                          }
                                    }
                              }
#endif
                        }
                  LD8("Seq::process -- n=%i (n = playPosFrame - *pPlayFrame = 'amount of frames to produce') ", n);
                  mabuf->utick = playPosUTick;
                  if (n) {
                        //if (cs->playMode() == PlayMode::SYNTHESIZER) {
                              metronome(n, pBuffer, inCountIn);
                              _synti->process(n, pBuffer);
                              pBuffer += n * 2;
                              *pPlayFrame  += n;
                              framesRemain -= n;
                              framePos     += n;
                        //      }
#if 0
                        else {
                              while (n > 0) {
                                    int section;
                                    float** pcm;
                                    long rn = ov_read_float(&vf, &pcm, n, &section);
                                    if (rn == 0)
                                          break;
                                    for (int i = 0; i < rn; ++i) {
                                          *pBuffer++ = pcm[0][i];
                                          *pBuffer++ = pcm[1][i];
                                          }
                                    *pPlayFrame  += rn;
                                    framesRemain -= rn;
                                    framePos     += rn;
                                    n            -= rn;
                                    }
                              }
#endif
                        }
                  // buffer has now been processed up to the framePos point. Now emit the event, which will be processed going forward from framePos
                  const NPlayEvent& event = (*pPlayPos)->second;
                  playEvent(event, framePos);

#if 0 //Disable metronome tick accent
                  if (event.type() == ME_TICK1) {
                        tickRemain = tickLength;
                        tickVolume = event.velo() ? qreal(event.value()) / 127.0 : 1.0;
                        }
                  else if (event.type() == ME_TICK2) {
                        tackRemain = tackLength;
                        tackVolume = event.velo() ? qreal(event.value()) / 127.0 : 1.0;
                        }
#endif
                  mutex.lock();
                  ++(*pPlayPos);
                  mutex.unlock();
                  }
            LD6("Seq::process --- framesRemain=%i", framesRemain);
            if (framesRemain) {
                  //if (cs->playMode() == PlayMode::SYNTHESIZER) {
                        metronome(framesRemain, pBuffer, inCountIn);
                        _synti->process(framesRemain, pBuffer);
                        *pPlayFrame += framesRemain;
                  //      }
                  //else {
#if 0
                        int n = framesRemain;
                        while (n > 0) {
                              int section;
                              float** pcm;
                              long rn = ov_read_float(&vf, &pcm, n, &section);
                              if (rn == 0)
                                    break;
                              for (int i = 0; i < rn; ++i) {
                                    *pBuffer++ = pcm[0][i];
                                    *pBuffer++ = pcm[1][i];
                                    }
                              *pPlayFrame  += rn;
                              framesRemain -= rn;
                              framePos     += rn;
                              n            -= rn;
                              }
#endif
                        //}
                  }
            if (*pPlayPos == pEventsEnd) {
                  if (inCountIn) {
#if 0
                        inCountIn = false;
                        // Connecting to JACK Transport if MuseScore was temporarily disconnected from it
                        if (useJackTransportSavedFlag) {
                              // Stopping fake driver
                              stopTransport();
                              preferences.setPreference(PREF_IO_JACK_USEJACKTRANSPORT, true);
                              // Starting the real JACK Transport. All applications play in sync now
                              startTransport();
                              }
#endif
                        }
                  else
                        stopTransport();
                  }
            }
      else {
#if 0 //FIX: tickLength and tackLength is unknown
            // Outside of playback mode
            while (!liveEventQueue()->empty()) {
                  const NPlayEvent& event = liveEventQueue()->dequeue();
                  if (event.type() == ME_TICK1) {
                        tickRemain = tickLength;
                        tickVolume = event.velo() ? qreal(event.value()) / 127.0 : 1.0;
                        }
                  else if (event.type() == ME_TICK2) {
                        tackRemain = tackLength;
                        tackVolume = event.velo() ? qreal(event.value()) / 127.0 : 1.0;
                        }
                  }
#endif
            if (framesRemain) {
                  metronome(framesRemain, pBuffer, true);
                  _synti->process(framesRemain, pBuffer);
                  }
            }
      //
      // metering / master gain
      //
      // Store this stat in the mabuf, which muxaudio the can report back, or if muxseq caches it
      qreal lv = 0.0f;
      qreal rv = 0.0f;
      pBuffer = buffer;
      for (unsigned i = 0; i < framesPerPeriod; ++i) {
            qreal val = *pBuffer;
            lv = qMax(lv, qAbs(val));
            pBuffer++;

            val = *pBuffer;
            rv = qMax(rv, qAbs(val));
            pBuffer++;
            }
      LD8("Seq::process buffer amp=%f/%f", lv, rv);
      meterValue[0] = lv;
      meterValue[1] = rv;
      if (meterPeakValue[0] < lv) {
            meterPeakValue[0] = lv;
            peakTimer[0] = 0;
            }
      if (meterPeakValue[1] < rv) {
            meterPeakValue[1] = rv;
            peakTimer[1] = 0;
            }
      }

//---------------------------------------------------------
//   initInstruments
//---------------------------------------------------------

void Seq::initInstruments(int newMaxMidiOutPort, int numSevs, struct SparseMidiEvent *sevs) {
    LD4("Seq::initInstruments newMaxMidiOutPort=%i maxMidiOutPort=%i", newMaxMidiOutPort, maxMidiOutPort);
    if (maxMidiOutPort < newMaxMidiOutPort) {
        maxMidiOutPort = newMaxMidiOutPort;
    }
    if (! g_driver_running) {
        return;
    }
    // Add midi out ports if necessary
    //FIX: causes jack to register ports stuck-in-a-loop
    muxseq_msg_to_audio(MsgTypeOutPortCount, maxMidiOutPort + 1);
    for (int i = 0; i < numSevs; i++) {
        struct SparseMidiEvent *sev = &sevs[i];
        NPlayEvent event(sev->type, sev->channel, sev->dataA, sev->dataB);
        event.syntiIdx = _synti->index(QString(sev->synthName));
        putEvent(event);
        LD6("Seq::initInstruments putEvent channel=%i type=%i dataA=%i dataB=%i synthName=%s", sev->channel, sev->type, sev->dataA, sev->dataB, sev->synthName);
    }
    LD6("Seq::initInstruments newMaxMidiOutPort=%i maxMidiOutPort=%i DONE", newMaxMidiOutPort, maxMidiOutPort);
}

void Seq::updateOutPortCount(const int portCount)
{
// FIX:      if (g_driver_running && (preferences.getBool(PREF_IO_JACK_USEJACKMIDI) || preferences.getBool(PREF_IO_ALSA_USEALSAAUDIO)))
//            muxseq_msg_to_audio(MsgTypeOutPortCount, portCount);
      }

//---------------------------------------------------------
//   renderChunk
//---------------------------------------------------------

//FIX: moved to muxseq_client

//---------------------------------------------------------
//   updateEventsEnd
//---------------------------------------------------------

void Seq::updateEventsEnd()
      {
      auto end = events.cend();
      eventsEnd = end;
      endUTick = events.empty() ? 0 : (--end)->first;
      }

//---------------------------------------------------------
//   collectEvents
//---------------------------------------------------------

void Seq::collectEvents(int utick)
      {
      //do not collect even while playing
      LD4("Seq::collectEvents utick=%i", utick);
      if (state == Transport::PLAY && playlistChanged)
            return;


      mutex.lock();

      if (midiRenderFuture.isRunning())
            midiRenderFuture.waitForFinished();

      // move any outstanding events in current eventmap
      if (playlistChanged) {
            midi.setScoreChanged();
            events.clear();
            renderEvents.clear();
            renderEventsStatus.clear();
            }
      else if (!renderEvents.empty()) {
            events.insert(renderEvents.begin(), renderEvents.end());
            renderEvents.clear();
            }

      struct MuxseqEventsHeader *meh = (struct MuxseqEventsHeader *) muxseq_mscore_query(MsgTypeSeqRenderEvents, utick);
      if (meh) {
          //FIX: make a helperfunction and put in muxlib (muxbuffer_to_eventMap)
          struct SparseEvent *sevs = meh->sevs;
          for (int i = 0; i < meh->numEvents; i++) {
              struct SparseEvent *sev = &sevs[i];
              int framepos = sev->framepos;
              NPlayEvent nev;
              nev.setType(sev->type);
              nev.setChannel(sev->channel);
              nev.setPitch(sev->pitch);
              nev.setVelo(sev->velo);
              nev.playPosSeconds = sev->playPosSeconds;
              nev.beatsPerSecond = sev->beatsPerSecond;
              nev.division = sev->division;
              nev.syntiIdx = _synti->index(QString(sev->synthName)); //FIX: resolve at setup midimapping at start _synti->index(cs->midiMapping(channel)->articulation()->synti());
              LD6("Seq::collectEvents on framepos %i set playPosSeconds=%f resolved synthName=%s to syntiIdx=%i", framepos, sev->playPosSeconds, sev->synthName, nev.syntiIdx);
              events.insert({framepos, nev});
          }
          free(meh);
      }
      LD4("Seq::collectEvents collected %i events", events.size());
      updateEventsEnd();
      //FIX: cant handle loops, need loop-tick from mscore: playPos = mscore->loop() ? events.find(cs->loopInTick().ticks()) : events.cbegin();
      playPos = events.cbegin();
      playlistChanged = false;
      mutex.unlock();
      }

//---------------------------------------------------------
//   ensureBufferAsync
//---------------------------------------------------------

void Seq::ensureBufferAsync(int utick)
      {
      if (mutex.tryLock()) { // sync with possible collectEvents calls

            if (midiRenderFuture.isRunning() || !allowBackgroundRendering) {
                  mutex.unlock();
                  return;
                  }

            if (!renderEvents.empty()) {
                  // TODO: use C++17 map::merge()?
                  events.insert(renderEvents.begin(), renderEvents.end());
                  updateEventsEnd();
                  renderEvents.clear();
                  }
#if 0 // FIX
            const int unrenderedUtick = renderEventsStatus.occupiedRangeEnd(utick);
            if (unrenderedUtick - utick < minUtickBufferSize) {
                  const MidiRenderer::Chunk chunk = midi.getChunkAt(unrenderedUtick);
                  if (chunk) {
                        midiRenderFuture = QtConcurrent::run([this, chunk]() {
                              renderChunk(chunk, &renderEvents);
                              });
                        }
                  }
#endif
            mutex.unlock();
            }
      }

//---------------------------------------------------------
//   getCurTick
//---------------------------------------------------------

int Seq::getCurTick()
      {
      LD("Seq::getCurTick ######## NOT IMPLEMENTED");
//FIX: FIXNOW, return cs->utime2utick(qreal(playFrame) / qreal(g_sampleRate));
      return 0;
      }

//---------------------------------------------------------
//   setRelTempo
//    relTempo = 1.0 = normal tempo
//---------------------------------------------------------

void Seq::setRelTempo(double relTempo)
      {
      guiToSeq(SeqMsg(SeqMsgId::TEMPO_CHANGE, relTempo));
      }

//---------------------------------------------------------
//   setPos
//    seek
//    realtime environment
//---------------------------------------------------------

void Seq::setPos(int utick)
      {
      LD("Seq::setPos utick=%u ######## NOT IMPLEMENTED");
      //FIX: if (cs == 0)
      //      return;
      stopNotes(-1, true);

      int ucur;
      mutex.lock();
#if 0 //FIX
      if (playPos != events.end())
            ucur = cs->repeatList().utick2tick(playPos->first);
      else
            ucur = utick - 1;
      if (utick != ucur)
            updateSynthesizerState(ucur, utick);

      playFrame = cs->utick2utime(utick) * g_sampleRate;
      playPos   = events.lower_bound(utick);
#endif
      mutex.unlock();
      }

//---------------------------------------------------------
//   getPlayStartUtick
//---------------------------------------------------------

int Seq::getPlayStartUtick()
      {
#if 0
      if ((mscore->loop())) { //FIX: mscore->loop access
            //FIX: if (preferences.getBool(PREF_APP_PLAYBACK_LOOPTOSELECTIONONPLAY)) {
            //      setLoopSelection();
            //      }
            // return cs->repeatList().tick2utick(cs->loopInTick().ticks());
            }
      //return cs->repeatList().tick2utick(cs->playPos().ticks());
#endif
      return 0;
      }

//---------------------------------------------------------
//   seekCommon
//   a common part of seek() and seekRT(), contains code
//   that could be safely called from any thread.
//   Do not use explicitly, use seek() or seekRT()
//---------------------------------------------------------

void Seq::seekCommon(int utick)
      {
      //FIX: if (cs == 0)
      //      return;
      LD("Seq::seekCommon utick=%i calling collectEvents");
      collectEvents(utick);
#if 0 // FIX
      if (cs->playMode() == PlayMode::AUDIO) {
            ogg_int64_t sp = cs->utick2utime(utick) * g_sampleRate;
            ov_pcm_seek(&vf, sp);
            }
#endif 

      guiPos = events.lower_bound(utick);
      //FIX: mscore->setPos(Fraction::fromTicks(cs->repeatList().utick2tick(utick)));
      unmarkNotes();
      }

//---------------------------------------------------------
//   seek
//   send seek message to sequencer
//   gui thread
//---------------------------------------------------------

void Seq::seek(int utick)
      {
      qDebug("-- Seq::seek utick=%i", utick);
#if 0 // FIX
      if (preferences.getBool(PREF_IO_JACK_USEJACKTRANSPORT)) {
            if (utick > endUTick)
                  utick = 0;
            msgToAudioSeekTransport(utick);
            if (utick != 0)
                  return;
            }
      seekCommon(utick);
      int t = cs->repeatList().utick2tick(utick);
      Segment* seg = cs->tick2segment(Fraction::fromTicks(t));
      if (seg)
            mscore->currentScoreView()->moveCursor(seg->tick());
      cs->setPlayPos(Fraction::fromTicks(t));
      cs->update();
      guiToSeq(SeqMsg(SeqMsgId::SEEK, utick));
#endif
      }

//---------------------------------------------------------
//   seekRT
//   realtime thread
//---------------------------------------------------------

void Seq::seekRT(int utick)
      {
      if (cachedPrefs.useJackTransport && utick > endUTick)
                  utick = 0;
      seekCommon(utick);
      setPos(utick);
      // Update the screen in GUI thread
//FIX:       emit toGui('5', cs->repeatList().utick2tick(utick));
      }

//---------------------------------------------------------
//   startNote
//---------------------------------------------------------

void Seq::startNote(int channel, int pitch, int velo, double nt)
      {
      if (state != Transport::STOP && state != Transport::PLAY)
            return;
      NPlayEvent ev(ME_NOTEON, channel, pitch, velo);
      ev.setTuning(nt);
      sendEvent(ev);
      }

void Seq::startNote(int channel, int pitch, int velo, int duration, double nt)
      {
      stopNotes();
      startNote(channel, pitch, velo, nt);
      startNoteTimer(duration);
      }

//---------------------------------------------------------
//   playMetronomeBeat
//---------------------------------------------------------

void Seq::playMetronomeBeat(BeatType type)
      {
      if (state != Transport::STOP)
            return;
      //FIX: no NPlayEvent type??? liveEventQueue()->enqueue(NPlayEvent(type));
      }

//---------------------------------------------------------
//   startNoteTimer
//---------------------------------------------------------

void Seq::startNoteTimer(int duration)
      {
      if (duration) {
            noteTimer->setInterval(duration);
            noteTimer->start();
            }
      }
//---------------------------------------------------------
//   stopNoteTimer
//---------------------------------------------------------

void Seq::stopNoteTimer()
      {
      if (noteTimer->isActive()) {
            noteTimer->stop();
            stopNotes();
            }
      }

//---------------------------------------------------------
//   stopNotes
//---------------------------------------------------------

void Seq::stopNotes(int channel, bool realTime)
      {
      auto send = [this, realTime](const NPlayEvent& event) {
            if (realTime)
                  putEvent(event);
            else
                  sendEvent(event);
            };
      // For VSTs/devices that do not support All Notes Off
      // CTRL_ALL_NOTES_OFF should still be evoked after calling this function, even if it seems redundant
      auto turnAllNotesOff = [send](int channel) {
            for (unsigned note = 0; note < 128; note++)
                  send(NPlayEvent(ME_NOTEOFF, channel, note, 0));
            };
      // Stop notes in all channels
      if (channel == -1) {
#if 0 //FIX: midimapping
            for(unsigned ch = 0; ch < cs->midiMapping().size(); ch++) {
                  send(NPlayEvent(ME_CONTROLLER, ch, CTRL_SUSTAIN, 0));
                  turnAllNotesOff(ch);
                  send(NPlayEvent(ME_CONTROLLER, ch, CTRL_ALL_NOTES_OFF, 0));
                  // if (cs->midiChannel(ch) != 9)
                  //       send(NPlayEvent(ME_PITCHBEND,  ch, 0, 64));
                  }
#endif
            }
      else {
            send(NPlayEvent(ME_CONTROLLER, channel, CTRL_SUSTAIN, 0));
            turnAllNotesOff(channel);
            send(NPlayEvent(ME_CONTROLLER, channel, CTRL_ALL_NOTES_OFF, 0));
            //FIX: if (cs->midiChannel(channel) != 9)
            //      send(NPlayEvent(ME_PITCHBEND,  channel, 0, 64));
            }
      if (cachedPrefs.useAlsaAudio || cachedPrefs.useJackAudio || cachedPrefs.usePulseAudio || cachedPrefs.usePortAudio) {
            guiToSeq(SeqMsg(SeqMsgId::ALL_NOTE_OFF, channel));
            }
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void Seq::setController(int channel, int ctrl, int data)
      {
      NPlayEvent event(ME_CONTROLLER, channel, ctrl, data);
      sendEvent(event);
      }

//---------------------------------------------------------
//   sendEvent
//    called from GUI context to send a midi event to
//    midi out or synthesizer
//---------------------------------------------------------

void Seq::sendEvent(const NPlayEvent& ev)
      {
      guiToSeq(SeqMsg(SeqMsgId::PLAY, ev));
      }

//---------------------------------------------------------
//   nextMeasure
//---------------------------------------------------------

void Seq::nextMeasure()
      {
#if 0 //FIX:
      Measure* m = cs->tick2measure(Fraction::fromTicks(guiPos->first));
      if (m) {
            if (m->nextMeasure())
                  m = m->nextMeasure();
            seek(m->tick().ticks());
            }
#endif
      }

//---------------------------------------------------------
//   nextChord
//---------------------------------------------------------

void Seq::nextChord()
      {
      int t = guiPos->first;
      for (auto i = guiPos; i != eventsEnd; ++i) {
            if (i->second.type() == ME_NOTEON && i->first > t && i->second.velo()) {
                  seek(i->first);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   prevMeasure
//---------------------------------------------------------

void Seq::prevMeasure()
      {
      auto i = guiPos;
      if (i == events.begin())
            return;
      --i;
#if 0
      Measure* m = cs->tick2measure(Fraction::fromTicks(i->first));
      if (m) {
            if ((i->first == m->tick().ticks()) && m->prevMeasure())
                  m = m->prevMeasure();
            seek(m->tick().ticks());
            }
#endif
      }

//---------------------------------------------------------
//   prevChord
//---------------------------------------------------------

void Seq::prevChord()
      {
      int t  = playPos->first;
      //find the chord just before playpos
#if 0 //FIX: query mscore, which can locate this
      EventMap::const_iterator i = events.upper_bound(cs->repeatList().tick2utick(t));
      for (;;) {
            if (i->second.type() == ME_NOTEON) {
                  const NPlayEvent& n = i->second;
                  if (i->first < t && n.velo()) {
                        t = i->first;
                        break;
                        }
                  }
            if (i == events.cbegin())
                  break;
            --i;
            }
      //go the previous chord
      if (i != events.cbegin()) {
            i = playPos;
            for (;;) {
                  if (i->second.type() == ME_NOTEON) {
                        const NPlayEvent& n = i->second;
                        if (i->first < t && n.velo()) {
                              seek(i->first);
                              break;
                              }
                        }
                  if (i == events.cbegin())
                        break;
                  --i;
                  }
            }
#endif
      }

//---------------------------------------------------------
//   seekEnd
//---------------------------------------------------------

void Seq::seekEnd()
      {
      qDebug("seek to end");
      }

//---------------------------------------------------------
//   guiToSeq
//---------------------------------------------------------

void Seq::guiToSeq(const SeqMsg& msg)
      {
#if 0 //FIX: use zmq instead
      if (!g_driver_running || !running)
            return;
      toSeq.enqueue(msg);
#endif
      }

//---------------------------------------------------------
//   eventToGui
//---------------------------------------------------------

void Seq::eventToGui(NPlayEvent e)
      {
//FIX: use zmq instead:      fromSeq.enqueue(SeqMsg(SeqMsgId::MIDI_INPUT_EVENT, e));
      }

//---------------------------------------------------------
//   midiInputReady
//---------------------------------------------------------

void Seq::midiInputReady()
      {
      //if (g_driver_running)
      //      muxseq_msg_to_audio(MsgTypeMidiInputReady, 0);
      }

#if 0
//---------------------------------------------------------
//   SeqMsgFifo
//---------------------------------------------------------
SeqMsgFifo::SeqMsgFifo()
      {
      maxCount = SEQ_MSG_FIFO_SIZE;
      clear();
      }

//---------------------------------------------------------
//   enqueue
//---------------------------------------------------------

void SeqMsgFifo::enqueue(const SeqMsg& msg)
      {
      int i = 0;
      int n = 50;
      QMutex mutex;
      QWaitCondition qwc;
      mutex.lock();
      for (; i < n; ++i) {
            if (!isFull())
                  break;
            qwc.wait(&mutex,100);
            }
      mutex.unlock();
      if (i == n) {
            qDebug("===SeqMsgFifo: overflow");
            return;
            }
      messages[widx] = msg;
      push();
      }

//---------------------------------------------------------
//   dequeue
//---------------------------------------------------------

SeqMsg SeqMsgFifo::dequeue()
      {
      SeqMsg msg = messages[ridx];
      pop();
      return msg;
      }
#endif

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void Seq::putEvent(const NPlayEvent& event, unsigned framePos)
      {
      //FIX: if (!cs)
      //      return;
      int channel = event.channel();
#if 0 //FIX
      if (channel >= load maxchannels int(cs->midiMapping().size())) {
            qDebug("bad channel value %d >= %d", channel, 1 /* int(cs->midiMapping().size())*/);
            return;
            }
#endif

      // audio
      int syntiIdx = event.syntiIdx;
      LD4("Seq::putEvent chan=%i idx=%i", channel, syntiIdx);
      _synti->play(event, syntiIdx);

      // send event to midi-output
      if (g_driver_running != 0 && (cachedPrefs.useJackMidi || cachedPrefs.useAlsaAudio || cachedPrefs.usePortAudio)) {
            // FIX: port and channel info must be stored in the event
            struct MuxaudioMsg msg;
            msg.type = MsgTypeEventToMidi;
            msg.payload.sparseMidiEvent.framepos = framePos;
            msg.payload.sparseMidiEvent.midiPort = event.midiPort;
            msg.payload.sparseMidiEvent.synthName[0] = 0;
            msg.payload.sparseMidiEvent.channel  = event.channel(); //score()->midiChannel(event.channel());
            msg.payload.sparseMidiEvent.type     = event.type();
            msg.payload.sparseMidiEvent.dataA    = event.dataA();
            msg.payload.sparseMidiEvent.dataB    = event.dataB();
            mux_zmq_ctrl_send_to_audio(msg);
            }
      }

//---------------------------------------------------------
//   heartBeat
//    update GUI
//---------------------------------------------------------

void Seq::heartBeatTimeout()
      {
#if 0 //FIX: move much functionallity to mscore side
      SynthControl* sc = mscore->getSynthControl();
      if (sc && g_driver_running) {
            if (++peakTimer[0] >= peakHold)
                  meterPeakValue[0] *= .7f;
            if (++peakTimer[1] >= peakHold)
                  meterPeakValue[1] *= .7f;
            //FIX: sc->setMeter(meterValue[0], meterValue[1], meterPeakValue[0], meterPeakValue[1]);
            }
      while (!fromSeq.empty()) {
            SeqMsg msg = fromSeq.dequeue();
            if (msg.id == SeqMsgId::MIDI_INPUT_EVENT) {
                  int type = msg.event.type();
                  if (type == ME_NOTEON)
                        mscore->midiNoteReceived(msg.event.channel(), msg.event.pitch(), msg.event.velo());
                  else if (type == ME_NOTEOFF)
                        mscore->midiNoteReceived(msg.event.channel(), msg.event.pitch(), 0);
                  else if (type == ME_CONTROLLER)
                        mscore->midiCtrlReceived(msg.event.controller(), msg.event.value());
                  }
            }
      if (state != Transport::PLAY || inCountIn)
            return;

      int endFrame = playFrame;

      mutex.lock();
      auto ppos = playPos;
      if (ppos != events.cbegin())
            --ppos;
      mutex.unlock();

      ensureBufferAsync(ppos->first);

      if (cs && cs->sigmap()->timesig(getCurTick()).nominal()!=prevTimeSig) {
            prevTimeSig = cs->sigmap()->timesig(getCurTick()).nominal();
            emit timeSigChanged();
            }
      if (cs && curTempo()!=prevTempo) {
            prevTempo = curTempo();
            emit tempoChanged();
            }
      QRectF r;
      for (;guiPos != eventsEnd; ++guiPos) {
            if (guiPos->first > ppos->first)
                  break;
            if (mscore->loop())
                  if (guiPos->first >= cs->repeatList().tick2utick(cs->loopOutTick().ticks()))
                        break;
            const NPlayEvent& n = guiPos->second;
            if (n.type() == ME_NOTEON) {
                  const Note* note1 = n.note();
                  if (n.velo()) {
                        while (note1) {
                              for (ScoreElement* se : note1->linkList()) {
                                    if (!se->isNote())
                                          continue;
                                    Note* currentNote = toNote(se);
                                    currentNote->setMark(true);
                                    markedNotes.append(currentNote);
                                    r |= currentNote->canvasBoundingRect();
                                    }
                              note1 = note1->tieFor() ? note1->tieFor()->endNote() : 0;
                              }
                        }
                  else {
                        while (note1) {
                              for (ScoreElement* se : note1->linkList()) {
                                    if (!se->isNote())
                                          continue;
                                    Note* currentNote = toNote(se);
                                    currentNote->setMark(false);
                                    r |= currentNote->canvasBoundingRect();
                                    markedNotes.removeOne(currentNote);
                                    }
                              note1 = note1->tieFor() ? note1->tieFor()->endNote() : 0;
                              }
                        }
                  }
            }
      int utick = ppos->first;
      int t = cs->repeatList().utick2tick(utick);
      mscore->currentScoreView()->moveCursor(Fraction::fromTicks(t));
      mscore->setPos(Fraction::fromTicks(t));

      //FIX: muxseq_send(SeqMsgHeartBeat);

      PianorollEditor* pre = mscore->getPianorollEditor();
      if (pre && pre->isVisible())
            pre->heartBeat(this);

      PianoTools* piano = mscore->pianoTools();
      if (piano && piano->isVisible())
            piano->updateAllKeys();

      //FIX: cv->update(cv->toPhysical(r));
#endif

      }


//---------------------------------------------------------
//   updateSynthesizerState
//    collect all controller events between tick1 and tick2
//    and send them to the synthesizer
//    Called from RT thread
//---------------------------------------------------------

void Seq::updateSynthesizerState(int tick1, int tick2)
      {
      if (tick1 > tick2)
            tick1 = 0;
      // Making a local copy of events to avoid touching it
      // from different threads at the same time
      EventMap ev = events;
      EventMap::const_iterator i1 = ev.lower_bound(tick1);
      EventMap::const_iterator i2 = ev.upper_bound(tick2);

      for (; i1 != i2; ++i1) {
            if (i1->second.type() == ME_CONTROLLER)
                  playEvent(i1->second, 0);
            }
      }

//---------------------------------------------------------
//   curTempo
//---------------------------------------------------------

double Seq::curTempo() const
      {
#if 0 //FIX
      if (playPos != events.end())
            return cs ? cs->tempomap()->tempo(playPos->first) : 0.0;
#endif
      return 0.0;
      }

//---------------------------------------------------------
//   set Loop in position
//---------------------------------------------------------

void Seq::setLoopIn()
      {
#if 0 //FIX
      Fraction t;
      if (state == Transport::PLAY) {     // If in playback mode, set the In position where note is being played
            auto ppos = playPos;
            if (ppos != events.cbegin())
                  --ppos;                 // We have to go back one pos to get the correct note that has just been played
            t = Fraction::fromTicks(cs->repeatList().utick2tick(ppos->first));
            }
      else
            t = cs->pos();        // Otherwise, use the selected note.
      if (t >= cs->loopOutTick())         // If In pos >= Out pos, reset Out pos to end of score
            cs->setPos(POS::RIGHT, cs->lastMeasure()->endTick());
      cs->setPos(POS::LEFT, t);
#endif
      }

//---------------------------------------------------------
//   set Loop Out position
//---------------------------------------------------------

void Seq::setLoopOut()
      {
#if 0 //FIX
      Fraction t;
      if (state == Transport::PLAY) {    // If in playback mode, set the Out position where note is being played
            t = Fraction::fromTicks(cs->repeatList().utick2tick(playPos->first));
            }
      else
            t = cs->pos() + cs->inputState().ticks();   // Otherwise, use the selected note.
      if (t <= cs->loopInTick())                        // If Out pos <= In pos, reset In pos to beginning of score
            cs->setPos(POS::LEFT, Fraction(0,1));
      else
          if (t > cs->lastMeasure()->endTick())
              t = cs->lastMeasure()->endTick();
      cs->setPos(POS::RIGHT, t);
      if (state == Transport::PLAY)
            guiToSeq(SeqMsg(SeqMsgId::SEEK, t.ticks()));
      }
#endif
}

void Seq::setPos(POS, unsigned t)
      {
      qDebug("seq: setPos %d", t);
      }

//---------------------------------------------------------
//   set Loop In/Out position based on the selection
//---------------------------------------------------------

void Seq::setLoopSelection()
      {
#if 0 //FIX
      const Score* score = mscore->currentScore();
      Q_ASSERT(!score || score->masterScore() == cs);

      if (score && score->selection().isRange()) {
            cs->setLoopInTick(score->selection().tickStart());
            cs->setLoopOutTick(score->selection().tickEnd());      
            }
      
      // add a dummy event to loop end if it is not already there
      // this is to let the playback reach the end completely before starting again
      if (!events.count(cs->loopOutTick().ticks())) {
            NPlayEvent ev;
            ev.setValue(ME_INVALID);
            events.insert(std::pair<int, Ms::NPlayEvent>(cs->loopOutTick().ticks(), ev));
            }
#endif
      }

//---------------------------------------------------------
//   Called after tempo or time signature
//   changed while playback
//---------------------------------------------------------

void Seq::handleTimeSigTempoChanged()
      {
      muxseq_msg_to_audio(MsgTypeTimeSigTempoChanged, 0);
      }

//---------------------------------------------------------
//  setInitialMillisecondTimestampWithLatency
//   Called whenever seq->process() starts.
//   Sets a starting reference time for which subsequent PortMidi events will be offset from.
//   Time is relative to the start of PortMidi's initialization.
//---------------------------------------------------------

void Seq::setInitialMillisecondTimestampWithLatency()
      {
#if 0 //FIX: portmidi sources are in muxaudio
     #ifdef USE_PORTMIDI
           initialMillisecondTimestampWithLatency = Pt_Time() + cachedPrefs.portMidiOutputLatencyMilliseconds;
           //qDebug("PortMidi initialMillisecondTimestampWithLatency: %d = %d + %d", initialMillisecondTimestampWithLatency, unsigned(Pt_Time()), preferences.getInt(PREF_IO_PORTMIDI_OUTPUTLATENCYMILLISECONDS));
     #endif
#endif
     }

//---------------------------------------------------------
//  getCurrentMillisecondTimestampWithLatency
//   Called when midi messages are sent to PortMidi device.
//   Returns the time in milliseconds of the current play cursor.
//   Time is relative to the start of PortMidi's initialization.
//---------------------------------------------------------

unsigned Seq::getCurrentMillisecondTimestampWithLatency(unsigned framePos) const
      {
//#if 0 // FIX: need sampleRate (a global that is sent from muxaudio)
#ifdef USE_PORTMIDI
      unsigned playTimeMilliseconds = unsigned(framePos * 1000) / unsigned(g_sampleRate);
      //qDebug("PortMidi timestamp = %d + %d", initialMillisecondTimestampWithLatency, playTimeMilliseconds);
      return initialMillisecondTimestampWithLatency + playTimeMilliseconds;
#else
      qDebug("Shouldn't be using this function if not using PortMidi");
      return 0;
#endif
//#endif
      }
}

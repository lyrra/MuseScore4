//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2010 Werner Schweer and others
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

#if (defined (_MSCVER) || defined (_MSC_VER))
// Include stdint.h and #define _STDINT_H to prevent <systemdeps.h> from redefining types
// #undef UNICODE to force LoadLibrary to use the char-based implementation instead of the wchar_t one.
#include <stdint.h>
#define _STDINT_H 1
#endif
#include <time.h>
#include "event.h"
#include "jackaudio.h"
#include "control.h"

#include <iostream>
#include <jack/midiport.h>

#include "libmscore/mscore.h"
//#include "mscore/musescore.h"
//#include "mscore/preferences.h"
#include "muxcommon.h"
#include "muxlib.h"
#include "muxaudio.h"

// Prevent killing sequencer with wrong data
#define less128(__less) ((__less >=0 && __less <= 127) ? __less : 0)

#define LL(str) std::cout << str << "\n"
#define LD(...) printf(__VA_ARGS__)

namespace Ms {

long g_jack_transport_position_time;
int g_ctrl_audio_error = 0;
int g_ctrl_audio_running = 0;
int g_muxseq_late = 0;
int g_muxseq_late_time = 0;

int debugMode = 1;
extern Driver* g_driver;
static jack_client_t* g_client; // used by static member function needing jack-client access
static Transport g_fakeState;
extern qreal g_utime;
extern qreal g_utick;
int g_mux_is_score_open = 1;
qreal g_utime = 0;
qreal g_utick = 0;
int g_sampleRate = 0;

struct {
    bool PREF_IO_JACK_USEJACKAUDIO = true;
    bool PREF_IO_JACK_USEJACKMIDI = true;
    bool PREF_IO_JACK_REMEMBERLASTCONNECTIONS = true;
    bool PREF_IO_JACK_TIMEBASEMASTER = false;
    bool PREF_IO_JACK_USEJACKTRANSPORT = false;
} preferences;

//FIX: move to muxlib
void mux_send_event (Event e) {
    struct SparseEvent se;
    se.type    = e.type();
    se.channel = e.channel();
    se.pitch   = e.pitch();
    se.velo    = e.velo();
    struct MuxaudioMsg msg;
    msg.type = MsgTypeEventToGui;
    memcpy(&msg.payload.sparseEvent, &se, sizeof(struct SparseEvent));
    muxaudio_mq_from_audio_writer_put(msg);
}

void mux_audio_init(int hot)
{
    g_driver->init(hot);
}

void mux_audio_start(int hotPlug)
{
    g_driver->start(hotPlug);
}

void mux_audio_stop()
{
}

void mux_audio_jack_transport_start() {
    qDebug("--- mux_audio_jack_transport_start\n");
    g_driver->startTransport();
}

void mux_audio_jack_transport_stop() {
    g_driver->stopTransport();
}

void mux_audio_jack_transport_seek(int utick) {
    g_driver->seekTransport(utick);
}

void mux_audio_handle_MsgTimeSigTempoChanged()
{
    g_driver->handleTimeSigTempoChanged();
}

void mux_audio_handle_updateOutPortCount(int portCount)
{
    g_driver->updateOutPortCount(portCount);
}

void mux_audio_send_event_to_midi(struct MuxaudioMsg msg) {
    NPlayEvent event;
    event.setType(msg.payload.sparseMidiEvent.type);
    event.setDataA(msg.payload.sparseMidiEvent.dataA);
    event.setDataB(msg.payload.sparseMidiEvent.dataB);
    g_driver->putEvent(event, msg.payload.sparseMidiEvent.framepos,
                              msg.payload.sparseMidiEvent.midiPort,
                              msg.payload.sparseMidiEvent.channel);
}


//---------------------------------------------------------
//   JackAudio
//---------------------------------------------------------

JackAudio::JackAudio()
   : Driver()
      {
      client = 0;
      }

//---------------------------------------------------------
//   ~JackAudio
//---------------------------------------------------------

JackAudio::~JackAudio()
      {
      if (client) {
            stop();
            if (jack_client_close(client)) {
                  qDebug("jack_client_close() failed: %s",
                     strerror(errno));
                  }
            }
      }

//---------------------------------------------------------
//   updateOutPortCount
//   Add/remove JACK MIDI Out ports
//---------------------------------------------------------

void JackAudio::updateOutPortCount(int maxport)
      {
      if (!preferences.PREF_IO_JACK_USEJACKMIDI || maxport == midiOutputPorts.size())
            return;
      std::cout << "JACK number of ports:" << midiOutputPorts.size() << ", change to:" << maxport << "\n";

      bool oldremember = preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS;
      preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS = true;

      if (maxport > midiOutputPorts.size()) {
            for (int i = midiOutputPorts.size(); i < maxport; ++i)
                  registerPort(QString("mscore-midi-%1").arg(i+1), false, true);
            restoreMidiConnections();
            }
      else if (maxport < midiOutputPorts.size()) {
            rememberMidiConnections();
            for(int i = midiOutputPorts.size() - 1; i >= maxport; --i) {
                  unregisterPort(midiOutputPorts[i]);
                  midiOutputPorts.removeAt(i);
                  }
            }
      preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS = oldremember;

      }

//---------------------------------------------------------
//   registerPort
//---------------------------------------------------------

void JackAudio::registerPort(const QString& name, bool input, bool midi)
      {
      int portFlag         = input ? JackPortIsInput : JackPortIsOutput;
      const char* portType = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;

      jack_port_t* port = jack_port_register(client, qPrintable(name), portType, portFlag, 0);
      if (port == 0) {
            qDebug("JackAudio:registerPort(%s) failed", qPrintable(name));
            return;
            }
      if (midi) {
            if (input)
                  midiInputPorts.append(port);
            else
                  midiOutputPorts.append(port);
            }
      else
            ports.append(port);
      }

//---------------------------------------------------------
//   unregisterPort
//---------------------------------------------------------

void JackAudio::unregisterPort(jack_port_t* port)
      {
      if (jack_port_is_mine(client,port)) {
            jack_port_unregister(client, port);
            port = 0;
            }
      else
            qDebug("Trying to unregister port that is not my!");
      }

//---------------------------------------------------------
//   inputPorts
//---------------------------------------------------------

QList<QString> JackAudio::inputPorts()
      {
      const char** prts = jack_get_ports(client, 0, 0, 0);
      QList<QString> clientList;
      for (const char** p = prts; p && *p; ++p) {
            jack_port_t* port = jack_port_by_name(client, *p);
            int flags = jack_port_flags(port);
            if (!(flags & JackPortIsInput))
                  continue;
            char buffer[128];
            strncpy(buffer, *p, sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = 0;
            if (strncmp(buffer, "Mscore", 6) == 0)
                  continue;
            clientList.append(QString(buffer));
            }
      return clientList;
      }

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void JackAudio::connect(void* src, void* dst)
      {
      const char* sn = jack_port_name((jack_port_t*) src);
      const char* dn = jack_port_name((jack_port_t*) dst);

      if (sn == 0 || dn == 0) {
            qDebug("JackAudio::connect: unknown jack ports");
            return;
            }
      if (jack_connect(client, sn, dn)) {
            qDebug("jack connect <%s>%p - <%s>%p failed",
               sn, src, dn, dst);
            }
      }

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void JackAudio::connect(const char* src, const char* dst)
      {
      if (src == 0 || dst == 0) {
            qDebug("JackAudio::connect: unknown jack ports");
            return;
            }
      qDebug("JackAudio::connect <%s> <%s>", src, dst);
      int rv = jack_connect(client, src, dst);
      if (rv)
            qDebug("jack connect port <%s> - <%s> failed: %d", src, dst, rv);
      }

//---------------------------------------------------------
//   disconnect
//---------------------------------------------------------

void JackAudio::disconnect(void* src, void* dst)
      {
      const char* sn = jack_port_name((jack_port_t*) src);
      const char* dn = jack_port_name((jack_port_t*) dst);
      if (sn == 0 || dn == 0) {
            qDebug("JackAudio::disconnect: unknown jack ports");
            return;
            }
      if (jack_disconnect(client, sn, dn)) {
            qDebug("jack disconnect <%s> - <%s> failed", sn, dn);
            }
      }

//---------------------------------------------------------
//   start
//    return false on error
//---------------------------------------------------------

bool JackAudio::start(bool hotPlug)
      {
      g_jack_transport_position_time = 0;
      bool oldremember = preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS;
      if (hotPlug)
            preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS = true;

      if (jack_activate(client)) {
            qDebug("JACK: cannot activate client");
            g_ctrl_audio_error = 1;
            return false;
            }
      /* connect the ports. Note: you can't do this before
         the client is activated, because we can't allow
         connections to be made to clients that aren't
         running.
       */
      if (preferences.PREF_IO_JACK_USEJACKAUDIO)
            restoreAudioConnections();
      if (preferences.PREF_IO_JACK_USEJACKMIDI)
            restoreMidiConnections();

      if (hotPlug)
            preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS = oldremember;
      muxaudio_msg_from_audio(MsgTypeAudioRunning, 1);
      return true;
      }

//---------------------------------------------------------
//   stop
//    return false on error
//---------------------------------------------------------

bool JackAudio::stop()
      {
      if (preferences.PREF_IO_JACK_USEJACKMIDI)
            rememberMidiConnections();
      if (preferences.PREF_IO_JACK_USEJACKAUDIO)
            rememberAudioConnections();

      if (jack_deactivate(client)) {
            qDebug("cannot deactivate client");
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   framePos
//---------------------------------------------------------

int JackAudio::framePos() const
      {
      jack_nframes_t n = jack_frame_time(client);
      return (int)n;
      }

//---------------------------------------------------------
//   freewheel_callback
//---------------------------------------------------------

static void freewheel_callback(int /*starting*/, void*)
      {
      }

//---------------------------------------------------------
//   sampleRateCallback
//---------------------------------------------------------

int sampleRateCallback(jack_nframes_t sampleRate, void*)
      {
      qDebug("JACK: sample rate changed: %d", sampleRate);
      //FIX: send samplerate to mux/seq
      //MScore::sampleRate = sampleRate;
      return 0;
      }

//---------------------------------------------------------
//   bufferSizeCallback called if JACK buffer changed
//---------------------------------------------------------

int bufferSizeCallback(jack_nframes_t nframes, void *arg)
      {
      JackAudio* audio = (JackAudio*)arg;
      audio->setBufferSize(nframes);
      return 0;
      }

static void registration_callback(jack_port_id_t, int, void*)
      {
//      qDebug("JACK: registration changed");
      }

static int graph_callback(void*)
      {
//      qDebug("JACK: graph changed");
      return 0;
      }

//---------------------------------------------------------
//   timebase
//---------------------------------------------------------

void JackAudio::timebase(jack_transport_state_t state, jack_nframes_t /*nframes*/, jack_position_t *pos, int /*new_pos*/, void *arg)
      {
/*
      JackAudio* audio = (JackAudio*)arg;




      if (!audio->seq->score()) {
            if (state==JackTransportLooping || state==JackTransportRolling)
                  audio->stopTransport();
            }
      else if (audio->seq->isRunning()) {
            if (!audio->seq->score()->masterScore())
                  return;

            pos->valid = JackPositionBBT;
            int curTick = audio->seq->score()->repeatList().utick2tick(audio->seq->getCurTick());
            int bar,beat,tick;
            audio->seq->score()->sigmap()->tickValues(curTick, &bar, &beat, &tick);
            // Providing the final tempo
            pos->beats_per_minute = 60 * audio->seq->curTempo() * audio->seq->score()->tempomap()->relTempo();
            pos->ticks_per_beat   = MScore::division;
            pos->tick             = tick;
            pos->bar              = bar+1;
            pos->beat             = beat+1;

            if (audio->timeSigTempoChanged) {
                  Fraction timeSig = audio->seq->score()->sigmap()->timesig(curTick).nominal();
                  pos->beats_per_bar =  timeSig.numerator();
                  pos->beat_type = timeSig.denominator();
                  audio->timeSigTempoChanged = false;
                  qDebug()<<"Time signature changed: "<< pos->beats_per_minute<<", bar: "<< pos->bar<<",beat: "<<pos->beat<<", tick:"<<pos->tick<<", time sig: "<<pos->beats_per_bar<<"/"<<pos->beat_type;
                  }
            }
      // TODO: Handle new_pos
*/
      }
//---------------------------------------------------------
//   processAudio
//    JACK callback
//---------------------------------------------------------

int JackAudio::processAudio(jack_nframes_t frames, void* p)
      {
      JackAudio* audio = (JackAudio*)p;
      if(! g_mux_is_score_open) {
            return 0;
            }

      float* l;
      float* r;
      if (preferences.PREF_IO_JACK_USEJACKAUDIO && audio->ports.size() == 2) {
            l = (float*)jack_port_get_buffer(audio->ports[0], frames);
            r = (float*)jack_port_get_buffer(audio->ports[1], frames);
            }
      else {
            l = 0;
            r = 0;
            }
      if (preferences.PREF_IO_JACK_USEJACKMIDI) {
            foreach(jack_port_t* port, audio->midiOutputPorts) {
                  void* portBuffer = jack_port_get_buffer(port, frames);
                  jack_midi_clear_buffer(portBuffer);
                  }
            foreach(jack_port_t* port, audio->midiInputPorts) {
                  void* portBuffer = jack_port_get_buffer(port, frames);
                  if (portBuffer) {
                        jack_nframes_t n = jack_midi_get_event_count(portBuffer);
                        for (jack_nframes_t i = 0; i < n; ++i) {
                              jack_midi_event_t event;
                              if (jack_midi_event_get(&event, portBuffer, i) != 0)
                                    continue;
                              size_t nn = event.size;
                              int type = event.buffer[0];
                              if (nn && (type == ME_CLOCK || type == ME_SENSE))
                                    continue;
                              Event e;
                              e.setChannel(type & 0xf);
                              type &= 0xf0;
                              e.setType(type);
                              if (type == ME_NOTEON || type == ME_NOTEOFF) {
                                    e.setPitch(event.buffer[1]);
                                    e.setVelo(event.buffer[2]);
                                    mux_send_event(e);
                                    }
                              else if (type == ME_CONTROLLER) {
                                    e.setController(event.buffer[1]);
                                    e.setValue(event.buffer[2]);
                                    mux_send_event(e);
                                    }
                              }
                        }
                  }
            }
#if (!defined (_MSCVER) && !defined (_MSC_VER))
         float buffer[frames * 2];
#else
         // MSVC does not support VLA. Replace with std::vector. If profiling determines that the
         //    heap allocation is slow, an optimization might be used.
         std::vector<float> vBuffer(frames * 2);
         float* buffer = vBuffer.data();
#endif
      {
          struct timespec tp;
          if (! clock_gettime(CLOCK_MONOTONIC, &tp)) {
              if ((tp.tv_nsec < g_jack_transport_position_time) || // wrap
                  (tp.tv_nsec - g_jack_transport_position_time) > 100000000) {
                  g_jack_transport_position_time = tp.tv_nsec;
                  jack_position_t pos;
                  jack_transport_query(g_client, &pos);
                  struct MuxaudioMsg msg;
                  //FIX: put the wallclock in the jacktransport message
                  msg.type = MsgTypeJackTransportPosition;
                  msg.payload.jackTransportPosition.state = static_cast<unsigned int>(getStateRT());
                  msg.payload.jackTransportPosition.frame = pos.frame;
                  msg.payload.jackTransportPosition.valid = pos.valid;
                  msg.payload.jackTransportPosition.beats_per_minute = pos.beats_per_minute;
                  msg.payload.jackTransportPosition.bbt = JackPositionBBT;
                  muxaudio_mq_from_audio_writer_put(msg);
              }
          }
      }
      // get audiochunk from mux/mscore-thread
      {
          struct timespec tp1, tp2;
          clock_gettime(CLOCK_MONOTONIC, &tp1);
          mux_process_bufferStereo((unsigned int)frames, buffer);
          clock_gettime(CLOCK_MONOTONIC, &tp2);
          unsigned int usec = (tp2.tv_sec - tp1.tv_sec) * 1000000;
          usec += (tp2.tv_nsec - tp1.tv_nsec) / 1000;
          if (usec > 500) {// report significant delay trying to get an audio buffer
              LD("AUDIO-DELAY %uus\n", usec);
              g_muxseq_late++;
              g_muxseq_late_time += usec;
          }
          //if (nsec > 100000) {
          //}
      }
      if (l && r) {
            float* sp = buffer;
            for (unsigned i = 0; i < frames; ++i) {
                  //FIX: dont interleave the buffer (done in seq/mux)
                  *l++ = *sp++;
                  *r++ = *sp++;
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   jackError
//---------------------------------------------------------

static void jackError(const char *s)
      {
      qDebug("JACK ERROR: %s", s);
      }

//---------------------------------------------------------
//   noJackError
//---------------------------------------------------------

static void noJackError(const char*  s )
      {
      qDebug("noJACK ERROR: %s", s);
      }

//---------------------------------------------------------
//   init
//    return false on error
//---------------------------------------------------------

bool JackAudio::init(bool hot)
      {
      g_driver = this;
      if (hot) {
            hotPlug();
            return true;
            }
      jack_set_error_function(noJackError);

      client = 0;
      timeSigTempoChanged = false;
      fakeState = Transport::STOP;
      g_fakeState = Transport::STOP;
      strcpy(_jackName, "mscore");

      jack_options_t options = (jack_options_t)0;
      jack_status_t status;
      g_client = client = jack_client_open(_jackName, options, &status);

      if (client == 0) {
            qDebug("JackAudio()::init(): failed, status 0x%0x", status);
            return false;
            }

      jack_set_error_function(jackError);
      jack_set_process_callback(client, processAudio, this);
      //jack_on_shutdown(client, processShutdown, this);
      jack_set_sample_rate_callback(client, sampleRateCallback, this);
      jack_set_port_registration_callback(client, registration_callback, this);
      jack_set_graph_order_callback(client, graph_callback, this);
      jack_set_freewheel_callback (client, freewheel_callback, this);
      if (preferences.PREF_IO_JACK_TIMEBASEMASTER)
            setTimebaseCallback();
      if (jack_set_buffer_size_callback (client, bufferSizeCallback, this) != 0)
            qDebug("Can not set bufferSizeCallback");
      _segmentSize  = jack_get_buffer_size(client);

      g_sampleRate = sampleRate();
      //FIX: send samplerate to mux/seq (which stores it in Mscore::sampleRate
      // register mscore left/right output ports
      if (preferences.PREF_IO_JACK_USEJACKAUDIO) {
            registerPort("left", false, false);
            registerPort("right", false, false);
            }

      if (preferences.PREF_IO_JACK_USEJACKMIDI) {
            registerPort(QString("mscore-midi-1"), false, true);
            registerPort(QString("mscore-midiin-1"), true, true);
            }
      return true;
      }

//---------------------------------------------------------
//   startTransport
//---------------------------------------------------------

void JackAudio::startTransport()
{
      if (preferences.PREF_IO_JACK_USEJACKTRANSPORT) {
            qDebug("    -- JackAudio::startTransport use REAL transport, set to start");
            jack_transport_start(client);
      } else {
            qDebug("    -- JackAudio::startTransport use g_fakeState, set to start");
            g_fakeState = fakeState = Transport::PLAY;
            }
}

//---------------------------------------------------------
//   stopTransport
//---------------------------------------------------------

void JackAudio::stopTransport()
      {
      if (preferences.PREF_IO_JACK_USEJACKTRANSPORT)
            jack_transport_stop(client);
      else
            g_fakeState = fakeState = Transport::STOP;
      }

//---------------------------------------------------------
//   getState
//---------------------------------------------------------
Transport JackAudio::getState()
      {
      return fakeState;
      }

Transport getStateRT()
      {
      if (!preferences.PREF_IO_JACK_USEJACKTRANSPORT) {
            return g_fakeState;
            }
      int transportState = jack_transport_query(g_client, NULL);
      switch (transportState) {
            case JackTransportStopped:  return Transport::STOP;
            case JackTransportLooping:
            case JackTransportRolling:  return Transport::PLAY;
            case JackTransportStarting: return Transport::PLAY;
            default:
                  return Transport::STOP;
            }
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void JackAudio::putEvent(const NPlayEvent& event, unsigned framePos, int portIdx, int channel)
      {
      if (!preferences.PREF_IO_JACK_USEJACKMIDI)
            return;

// qDebug("JackAudio::putEvent %d:%d  pos %d(%d)", portIdx, channel, framePos, _segmentSize);

      if (portIdx < 0 || portIdx >= midiOutputPorts.size()) {
            qDebug("JackAudio::putEvent: invalid port %d", portIdx);
            return;
            }
      jack_port_t* port = midiOutputPorts[portIdx];
      /*
      if (midiOutputTrace) {
            const char* portName = jack_port_name(port);
            int a     = event.dataA();
            int b     = event.dataB();
            LOG("MidiOut<%s>: jackMidi: %02x %02x %02x, channel: %i", portName, event.type(), a, b, channel);
            // e.dump();
            }
      */
      void* pb = jack_port_get_buffer(port, _segmentSize);

      if (pb == NULL) {
            qDebug()<<"jack_port_get_buffer failed, cannot send anything";
            }

      if (framePos >= _segmentSize) {
            qDebug("JackAudio::putEvent: time out of range %d(seg=%d)", framePos, _segmentSize);
            if (framePos > _segmentSize)
                  framePos = _segmentSize - 1;
            }

      switch(event.type()) {
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_POLYAFTER:
            case ME_CONTROLLER:
                  // Catch CTRL_PROGRAM and let other ME_CONTROLLER events to go
                  if (event.dataA() == CTRL_PROGRAM) {
                        // Convert CTRL_PROGRAM event to ME_PROGRAM
                        unsigned char* p = jack_midi_event_reserve(pb, framePos, 2);
                        if (p == 0) {
                              qDebug("JackMidi: buffer overflow, event lost");
                              return;
                              }
                        p[0] = ME_PROGRAM | channel;
                        p[1] = less128(event.dataB());
                        break;
                        }
                  //fall through
            case ME_PITCHBEND:
                  {
                  unsigned char* p = jack_midi_event_reserve(pb, framePos, 3);
                  if (p == 0) {
                        qDebug("JackMidi: buffer overflow, event lost");
                        return;
                        }
                  p[0] = event.type() | channel;
                  p[1] = less128(event.dataA());
                  p[2] = less128(event.dataB());
                  }
                  break;

            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  {
                  unsigned char* p = jack_midi_event_reserve(pb, framePos, 2);
                  if (p == 0) {
                        qDebug("JackMidi: buffer overflow, event lost");
                        return;
                        }
                  p[0] = event.type() | channel;
                  p[1] = less128(event.dataA());
                  }
                  break;
          // Do we really need to handle ME_SYSEX?
          /*  case ME_SYSEX:
                  {
                  const unsigned char* data = e.edata();
                  int len = e.len();
                  unsigned char* p = jack_midi_event_reserve(pb, framePos, len+2);
                  if (p == 0) {
                        qDebug("JackMidi: buffer overflow, event lost");
                        return;
                        }
                  p[0]     = 0xf0;
                  p[len+1] = 0xf7;
                  memcpy(p+1, data, len);
                  }
                  break;*/
            case ME_SONGPOS:
            case ME_CLOCK:
            case ME_START:
            case ME_CONTINUE:
            case ME_STOP:
                  qDebug("JackMidi: event type %x not supported", event.type());
                  break;
            }
      }

//---------------------------------------------------------
//   midiRead
//---------------------------------------------------------

void JackAudio::midiRead()
      {
//      midiDriver->read();
      }

//---------------------------------------------------------
//   handleTimeSigTempoChanged
//   Called after tempo or time signature
//   changed while playback
//---------------------------------------------------------

void JackAudio::handleTimeSigTempoChanged()
      {
      timeSigTempoChanged = true;
      }


//---------------------------------------------------------
//   seekTransport
//---------------------------------------------------------

void JackAudio::seekTransport(int utick)
      {
      if (debugMode)
            std::cout <<"jack locate to utick: " << g_utick << ", frame: " << int(g_utime * g_sampleRate) << "\n";
      jack_transport_locate(client, int(g_utime * g_sampleRate));
      }

//---------------------------------------------------------
//   setTimebaseCallback
//---------------------------------------------------------

void JackAudio::setTimebaseCallback()
      {
      int errCode = jack_set_timebase_callback(client, 0, timebase, this); // 0: force set timebase
      if (errCode == 0) {
            if (debugMode)
                  std::cout << "Registered as JACK Timebase Master.\n";
            }
      else {
            preferences.PREF_IO_JACK_TIMEBASEMASTER = false;
            qDebug("Unable to take over JACK Timebase, error code: %i",errCode);
            }
      }

//---------------------------------------------------------
//   releaseTimebaseCallback
//---------------------------------------------------------

void JackAudio::releaseTimebaseCallback()
      {
      int errCode = jack_release_timebase(client);
      if (errCode == 0)
            qDebug("Unregistered as JACK Timebase Master");
      else
            qDebug("Unable to unregister as JACK Timebase Master (not a Timebase Master?), error code: %i", errCode);
      }

//---------------------------------------------------------
//   rememberAudioConnections
//---------------------------------------------------------

void JackAudio::rememberAudioConnections()
      {
      if (!preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS)
            return;
      if (debugMode)
            LL("Saving audio connections...");
      QSettings settings;
      settings.setValue(QString("audio-0-connections"), 0);
      settings.setValue(QString("audio-1-connections"), 0);
      int port = 0;
      foreach(jack_port_t* mp, ports) {
            const char** cc = jack_port_get_connections(mp);
            const char** c = cc;
            int idx = 0;
            while (c) {
                  const char* p = *c++;
                  if (p == 0)
                        break;
                  settings.setValue(QString("audio-%1-%2").arg(port).arg(idx), p);
                  ++idx;
                  }
            settings.setValue(QString("audio-%1-connections").arg(port), idx);
            free((void*)cc);
            ++port;
            }
      }

//---------------------------------------------------------
//   restoreAudioConnections
//   Connect to the ports in Preferences->I/O
//---------------------------------------------------------

void JackAudio::restoreAudioConnections()
      {
      for (auto p : qAsConst(ports))
            jack_port_disconnect(client, p);

      QList<QString> portList = inputPorts();
      QList<QString>::iterator pi = portList.begin();

      QSettings settings;
      // Number of saved ports
      int n = settings.value(QString("audio-0-connections"), 0).toInt() + settings.value(QString("audio-1-connections"), 0).toInt();

      // Connecting to system ports
      if (!preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS || n == 0) {
            if (debugMode)
                LL("Connecting to system ports...");
            for (auto p : qAsConst(ports)) {
                  const char* src = jack_port_name(p);
                  if (pi != portList.end()) {
                        connect(src, qPrintable(*pi));
                        ++pi;
                        }
                  }
            return;
            }
      if (debugMode)
          LL("Restoring audio connections...");
      // Connecting to saved ports
      int nPorts = ports.size();
      for (int i = 0; i < nPorts; ++i) {
            int j = settings.value(QString("audio-%1-connections").arg(i), 0).toInt();
            const char* src = jack_port_name(ports[i]);
            for (int k = 0; k < j; ++k) {
                  QString dst = settings.value(QString("audio-%1-%2").arg(i).arg(k), "").toString();
                  if (!dst.isEmpty()) {
                        if (jack_port_connected_to(ports[i], qPrintable(dst)))
                              qDebug()<<"Audio port "<<src<<" ("<<i<<") already connected to "<<qPrintable(dst);
                        else
                              connect(src, qPrintable(dst));
                        }
                  }
            }
      }

//---------------------------------------------------------
//   rememberMidiConnections
//---------------------------------------------------------

void JackAudio::rememberMidiConnections()
      {
      if (!preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS)
            return;
      if (debugMode)
          LL("Saving midi connections...");
      QSettings settings;
      int port = 0;
      foreach(jack_port_t* mp, midiOutputPorts) {
            const char** cc = jack_port_get_connections(mp);
            const char** c = cc;
            int idx = 0;
            while (c) {
                  const char* p = *c++;
                  if (p == 0)
                        break;
                  settings.setValue(QString("midi-%1-%2").arg(port).arg(idx), p);
                  ++idx;
                  }
            settings.setValue(QString("midi-%1-connections").arg(port), idx);
            free((void*)cc);
            ++port;
            }

      port = 0;
      foreach(jack_port_t* mp, midiInputPorts) {
            const char** cc = jack_port_get_connections(mp);
            const char** c = cc;
            int idx = 0;
            while (c) {
                  const char* p = *c++;
                  if (p == 0)
                        break;
                  settings.setValue(QString("midiin-%1-%2").arg(idx).arg(port), p);
                  ++idx;
                  }
            settings.setValue(QString("midiin-%1-connections").arg(port), idx);
            free((void*)cc);
            ++port;
            }
      }

//---------------------------------------------------------
//   restoreMidiConnections
//   Connects to the ports from previous connection
//---------------------------------------------------------

void JackAudio::restoreMidiConnections()
      {
      if (!preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS)
            return;
      if (debugMode)
          LL("Restoring midi connections...");
      QSettings settings;
      int nPorts = midiOutputPorts.size();
      for (int i = 0; i < nPorts; ++i) {
            int n = settings.value(QString("midi-%1-connections").arg(i), 0).toInt();
            const char* src = jack_port_name(midiOutputPorts[i]);
            for (int k = 0; k < n; ++k) {
                  QString dst = settings.value(QString("midi-%1-%2").arg(i).arg(k), "").toString();
                  if (!dst.isEmpty()) {
                        if (jack_port_connected_to(midiOutputPorts[i], qPrintable(dst)))
                              continue;
                        connect(src, qPrintable(dst));
                        }
                  }
            }
      nPorts = midiInputPorts.size();
      for (int i = 0; i < nPorts; ++i) {
            int n = settings.value(QString("midiin-%1-connections").arg(i), 0).toInt();
            const char* dst = jack_port_name(midiInputPorts[i]);
            for (int k = 0; k < n; ++k) {
                  QString src = settings.value(QString("midiin-%1-%2").arg(k).arg(i), "").toString();
                  if (!src.isEmpty()) {
                        if (jack_port_connected_to(midiInputPorts[i], qPrintable(src)))
                              continue;
                        connect(qPrintable(src), dst);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   hotPlug
//   Change driver settings without unload
//---------------------------------------------------------

void JackAudio::hotPlug()
      {
      bool oldremember = preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS;
      preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS = true;
      // Remember connections before calling jack_deactivate() - it disconnects all ports
      rememberMidiConnections();
      if (ports.size() != 0)
            rememberAudioConnections();

      // We must set callbacks only on inactive client
      if (jack_deactivate(client))
            qDebug("cannot deactivate client");

      // Audio connections
      if (preferences.PREF_IO_JACK_USEJACKAUDIO) {
            if (ports.size() == 0) {
                  registerPort("left", false, false);
                  registerPort("right", false, false);
                  }
            }
      else if (!preferences.PREF_IO_JACK_USEJACKAUDIO) {
            foreach(jack_port_t* p, ports) {
                  unregisterPort(p);
                  ports.removeOne(p);
                  }
            }

      // Midi connections
      if (preferences.PREF_IO_JACK_USEJACKMIDI) {
            if (midiInputPorts.size() == 0)
                  registerPort(QString("mscore-midiin-1"), true, true);
            }
      else { // No midi
            updateOutPortCount(0);
            if (midiInputPorts.size() != 0) {
                  unregisterPort(midiInputPorts[0]);
                  midiInputPorts.removeOne(midiInputPorts[0]);
                  }
            }

      // Timebase Master callback
      if (preferences.PREF_IO_JACK_TIMEBASEMASTER)
            setTimebaseCallback();
      else
            releaseTimebaseCallback();

      preferences.PREF_IO_JACK_REMEMBERLASTCONNECTIONS = oldremember;
      }
}

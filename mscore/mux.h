/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#ifndef __MUX_H__
#define __MUX_H__


namespace Ms {

// this are also the jack audio transport states:
enum class Transport : char {
      STOP=0,
      PLAY=1,
      STARTING=3,
      NET_STARTING=4
      };

/*** MUX public interface ***/
int mux_is_score_open ();
void mux_send_event (Event e);
void mux_process_bufferStereo(unsigned int numFrames, float* bufferStereo);
void mux_set_jack_transport(Transport transport);
/****************************/

/*
 * message queue, between audio and mux
 */

enum MsgType {
    MsgTypeInit = 0,
    MsgTypeTransportStart,
    MsgTypeTransportStop,
    MsgTypeTransportSeek,
    MsgTypeJackTransportPosition,
    MsgTypeEventToGui,
    MsgTypeEventToMidi,
    MsgTypeNoop
};

struct JackTransportPosition {
    unsigned int frame;
    unsigned int valid;
    unsigned int beats_per_minute;
    unsigned int bbt;
};

struct SparseEvent {
    uchar type;
    uchar channel;
    int pitch;
    int velo;
    int cont;
    int val;
};

struct SparseMidiEvent {
    unsigned int framepos;
    int portIdx;
    int channel;
    uchar type;
    int dataA;
    int dataB;
};

struct Msg {
    MsgType type;
    union Payload {
        int i;
        SparseEvent sparseEvent;
        SparseMidiEvent sparseMidiEvent;
        struct JackTransportPosition jackTransportPosition;
    } payload;
};

int mux_mq_from_audio_writer_put (struct Msg msg);
int mux_mq_to_audio_writer_put (struct Msg msg);
int mux_mq_to_audio_visit();

void mux_set_jack_position(struct JackTransportPosition jackTransportPosition);
void mux_audio_jack_transport_start();
void mux_audio_jack_transport_stop();
void mux_audio_jack_transport_seek(int utick);

} // namespace Ms
#endif

/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#ifndef __MUXAUDIO_H__
#define __MUXAUDIO_H__


namespace Ms {

/*** MUX public interface ***/
int mux_is_score_open ();
//void mux_send_event (Event e);
void mux_process_bufferStereo(unsigned int numFrames, float* bufferStereo);
void mux_set_jack_transport(Transport transport);
/****************************/

/*
 * message queue, between audio and mux
 */

enum MsgType {
    MsgTypeInit = 0,
    MsgTypeAudioInit,
    MsgTypeAudioStart,
    MsgTypeAudioStop,
    MsgTypeAudioRunning,
    MsgTypeTransportStart,
    MsgTypeTransportStop,
    MsgTypeTransportSeek,
    MsgTypeJackTransportPosition,
    MsgTypeEventToGui,
    MsgTypeEventToMidi,
    MsgTypeTimeSigTempoChanged,
    MsgTypeOutPortCount,
    MsgTypeNoop
};

struct JackTransportPosition {
    unsigned int state;
    unsigned int frame;
    unsigned int valid;
    unsigned int beats_per_minute;
    unsigned int bbt;
};

struct SparseEvent {
    unsigned char type;
    unsigned char channel;
    int pitch;
    int velo;
    int cont;
    int val;
};

struct SparseMidiEvent {
    unsigned int framepos;
    int portIdx;
    int channel;
    unsigned char type;
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

void mux_msg_from_audio(MsgType typ, int val);
void mux_msg_to_audio(MsgType typ, int val);

void mux_audio_init(int hot);
void mux_audio_start(int hotPlug);
void mux_audio_stop();
void mux_set_jack_position(struct JackTransportPosition jackTransportPosition);
void mux_audio_jack_transport_start();
void mux_audio_jack_transport_stop();
void mux_audio_jack_transport_seek(int utick);
void mux_audio_handle_MsgTimeSigTempoChanged();
void mux_audio_handle_updateOutPortCount(int portCount);

} // namespace Ms
#endif

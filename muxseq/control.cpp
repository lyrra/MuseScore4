/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */
/*
 *
 */

#include "config.h"
#include <thread>
#include <chrono>
#include <iostream>
#include "event.h"
#include "mux.h"
#include "muxcommon.h"
#include "muxlib.h"
#include "muxseq.h"
#include "muxqueue.h"

#define MAILBOX_SIZE 256

namespace Ms {

void muxseq_mscoreQueryServer_mainloop(Mux::MuxSocket &sock);
void muxseq_mscoreQueryReqServer_mainloop(Mux::MuxSocket &sock);
void mux_network_server_ctrl();
void mux_network_server_audio();
int mux_mq_to_audio_visit();
void mux_audio_process();

void muxseq_muxaudioQueryClient_mainloop(Mux::MuxSocket &sock);
//void muxseq_muxaudioWorker_thread_init(std::string msg);
//void muxseq_audioWorker_thread_init(std::string msg);
void muxseq_muxaudioWorker_process();

static std::vector<std::thread> muxThreads;
struct Mux::MuxSocket g_muxsocket_mscoreQueryServer;
struct Mux::MuxSocket g_muxsocket_mscoreQueryReqServer;
struct Mux::MuxSocket g_muxsocket_muxaudioQueryClientAudio;
struct Mux::MuxSocket g_muxsocket_muxaudioQueryClientCtrl;
extern int g_muxseq_audio_process_run;
extern struct MuxQueue *queue_from_mscore;

/* this thread listens on message from musescore */
void muxseq_mscoreQueryServer_thread_init(std::string _notused)
{
    _set_threadname("m>  ");
    Mux::mux_make_connection(g_muxsocket_mscoreQueryServer, MUX_MUSESCORE_QUERY_SERVER_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REP, Mux::ZmqServer::BIND);
    muxseq_mscoreQueryServer_mainloop(g_muxsocket_mscoreQueryServer);
}

/* this thread ask message to musescore */
void muxseq_mscoreQueryReqServer_thread_init(std::string _notused)
{
    _set_threadname("m<  ");
    Mux::mux_make_connection(g_muxsocket_mscoreQueryReqServer, MUX_MUSESCORE_QUERYREQ_SERVER_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REQ, Mux::ZmqServer::BIND);
    muxseq_mscoreQueryReqServer_mainloop(g_muxsocket_mscoreQueryReqServer);
}


/* this thread listens on message from muxaudio */
void muxseq_muxaudioQueryClient_thread_init(std::string _notused)
{
    _set_threadname("<a  ");
    Mux::mux_make_connection(g_muxsocket_muxaudioQueryClientAudio, MUX_MUXAUDIO_QUERY_AUDIO_CLIENT_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REP, Mux::ZmqServer::CONNECT);
    muxseq_muxaudioQueryClient_mainloop(g_muxsocket_muxaudioQueryClientAudio);
}

void muxseq_thread_process_init(std::string msg)
{
    std::cout << "MUXSEQ audio-process thread initialized:" << msg << "\n";
    //muxseq_audio_process();
}

void muxseq_muxaudioWorker_thread_init(std::string msg)
{
    _set_threadname("wrk ");
    LD("MUXSEQ audio-worker-process thread initializing.");
    muxseq_muxaudioWorker_process();
}

void muxseq_audioQueryServer_thread_init(std::string msg)
{
    LD("MUXSEQ audio-query-server thread initializing.");
}

void muxseq_stop_threads()
{
    LD("MUXSEQ stop audio threads");
    g_muxseq_audio_process_run = 0;
    //seqThreads[0].join();
}

void muxseq_network_reader_thread_init(std::string _notused)
{
    //muxseq_network_server_ctrl();
}

void muxseq_audio_zmq_connect()
{
    Mux::mux_make_connection(g_muxsocket_muxaudioQueryClientCtrl, MUX_MUXAUDIO_QUERY_CTRL_CLIENT_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REQ, Mux::ZmqServer::CONNECT);
}


void muxseq_threads_start()
{
    queue_from_mscore = mux_mq_new(MAILBOX_SIZE);
    if (! queue_from_mscore) {
        LE("Failed to allocate memory\n");
        exit(1);
    }
    LD("start threads\n");
    std::vector<std::thread> threadv;

    // phase one, connect to muxaudio

    muxseq_audio_zmq_connect();

    std::thread procThread(muxseq_muxaudioQueryClient_thread_init, "notused");
    threadv.push_back(std::move(procThread));

    std::thread workerThread(muxseq_muxaudioWorker_thread_init, "notused");
    threadv.push_back(std::move(workerThread));

    //FIX: wait for servers to have connected to muxaudio

    // phase two, start network serving musescore

    std::thread mscoreQueryServerThread(muxseq_mscoreQueryServer_thread_init, "notused");
    threadv.push_back(std::move(mscoreQueryServerThread));

    std::thread mscoreQueryReqServerThread(muxseq_mscoreQueryReqServer_thread_init, "notused");
    threadv.push_back(std::move(mscoreQueryReqServerThread));

    muxThreads = std::move(threadv);
}

void muxseq_threads_stop()
{
    std::cout << "MUXSEQ stop all threads\n";
    //muxThreads[0].join();
}

} // end of Ms namespace



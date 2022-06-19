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
#include "muxaudio/muxaudio.h"
#include "muxseq.h"

namespace Ms {

void muxseq_mscoreQueryServer_mainloop(Mux::MuxSocket &sock);
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
struct Mux::MuxSocket g_muxsocket_muxaudioQueryClient;
extern int g_muxseq_audio_process_run;

/* this thread listens on message from musescore */
void muxseq_mscoreQueryServer_thread_init(std::string _notused)
{
    Mux::mux_make_connection(g_muxsocket_mscoreQueryServer, MUX_MUSESCORE_QUERY_SERVER_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REP, Mux::ZmqServer::BIND);
    muxseq_mscoreQueryServer_mainloop(g_muxsocket_mscoreQueryServer);
}

/* this thread listens on message from muxaudio */
void muxseq_muxaudioQueryClient_thread_init(std::string _notused)
{
    Mux::mux_make_connection(g_muxsocket_muxaudioQueryClient, MUX_AUDIO_QUERY_CLIENT_URL, Mux::ZmqType::QUERY, Mux::ZmqDir::REP, Mux::ZmqServer::CONNECT);
    muxseq_muxaudioQueryClient_mainloop(g_muxsocket_muxaudioQueryClient);
}

void muxseq_thread_process_init(std::string msg)
{
    std::cout << "MUXSEQ audio-process thread initialized:" << msg << "\n";
    //muxseq_audio_process();
}

void muxseq_muxaudioWorker_thread_init(std::string msg)
{
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

void muxseq_audio_zmq_thread_init(std::string _notused)
{
    //muxseq_network_server_audio();
}


void muxseq_threads_start()
{
    LD("start threads\n");
    std::vector<std::thread> threadv;

    std::thread mscoreQueryServerThread(muxseq_mscoreQueryServer_thread_init, "notused");
    threadv.push_back(std::move(mscoreQueryServerThread));

    std::thread procThread(muxseq_muxaudioQueryClient_thread_init, "notused");
    threadv.push_back(std::move(procThread));

    std::thread workerThread(muxseq_muxaudioWorker_thread_init, "notused");
    threadv.push_back(std::move(workerThread));

//    std::thread zmqCtrlThread(muxseq_network_reader_thread_init, "notused");
//    threadv.push_back(std::move(zmqCtrlThread));

//    std::thread zmqAudioThread(muxseq_audio_zmq_thread_init, "notused");
//    threadv.push_back(std::move(zmqAudioThread));

    muxThreads = std::move(threadv);
}

void muxseq_threads_stop()
{
    std::cout << "MUXSEQ stop all threads\n";
    //muxThreads[0].join();
}

} // end of Ms namespace



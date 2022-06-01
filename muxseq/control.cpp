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

void muxseq_audioQueryClient_mainloop(Mux::MuxSocket &sock);
void muxseq_audioWorker_thread_init(std::string msg);

static std::vector<std::thread> muxThreads;
struct Mux::MuxSocket g_muxsocket_mscoreQueryServer;
struct Mux::MuxSocket g_muxsocket_audioQueryClient;

void muxseq_mscoreQueryServer_thread_init(std::string _notused)
{
    Mux::mux_network_query_server(g_muxsocket_mscoreQueryServer, MUX_MUSESCORE_QUERY_SERVER_URL, false);
    muxseq_mscoreQueryServer_mainloop(g_muxsocket_mscoreQueryServer);
}

void muxseq_audioQueryClient_thread_init(std::string _notused)
{
    Mux::mux_network_query_client(g_muxsocket_audioQueryClient, MUX_AUDIO_QUERY_CLIENT_URL, false);
    muxseq_audioQueryClient_mainloop(g_muxsocket_audioQueryClient);
}

void muxseq_thread_process_init(std::string msg)
{
    std::cout << "MUXSEQ audio-process thread initialized:" << msg << "\n";
    //muxseq_audio_process();
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

    std::thread procThread(muxseq_audioQueryClient_thread_init, "notused");
    threadv.push_back(std::move(procThread));

    std::thread workerThread(muxseq_audioWorker_thread_init, "notused");
    threadv.push_back(std::move(workerThread));

//    threadv.push_back(std::move(procThread));

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



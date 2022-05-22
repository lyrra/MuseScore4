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

namespace Ms {

void mux_network_server_ctrl();
void mux_network_server_audio();
int mux_mq_to_audio_visit();
void mux_audio_process();

static std::vector<std::thread> muxThreads;

void muxseq_audio_control_thread_init(std::string _notused)
{
    while (1) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        // if (! mux_mq_to_audio_visit()) {}
    }
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
    fprintf(stderr, "start threads\n"); std::fflush(stderr);
    std::vector<std::thread> threadv;

    //muxseq_network_open();

//    std::thread ctrlThread(muxseq_control_thread_init, "notused");
//    threadv.push_back(std::move(ctrlThread));

//    std::thread procThread(muxseq_process_thread_init, "notused");
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



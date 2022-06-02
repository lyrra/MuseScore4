/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */
/*
 *
 */

#include "config.h"
#include <thread> // FIX: should be imported by config.h
#include <chrono>
#include <iostream>

namespace Ms {

void muxaudio_network_server_audio();
int mux_mq_to_audio_visit();
void muxaudio_audio_process();
void muxaudio_audio_process_stop ();
void muxaudio_network_server_ctrl();

static std::vector<std::thread> muxThreads;

void mux_thread_process_init(std::string msg)
{
    std::cout << "MUX audio-process thread initialized:" << msg << "\n";
    muxaudio_audio_process();
}

/* this threads replies to queries from muxseq */
void mux_ctrl_zmq_thread_init(std::string _notused)
{
    muxaudio_network_server_ctrl();
}

/* this threads send queries to muxseq */
void mux_audio_zmq_thread_init(std::string _notused)
{
    muxaudio_network_server_audio();
}


void muxaudio_threads_start()
{
    std::vector<std::thread> threadv;
    std::thread procThread(mux_thread_process_init, "notused");
    threadv.push_back(std::move(procThread));
    std::thread zmqCtrlThread(mux_ctrl_zmq_thread_init, "notused");
    threadv.push_back(std::move(zmqCtrlThread));
    std::thread zmqAudioThread(mux_audio_zmq_thread_init, "notused");
    threadv.push_back(std::move(zmqAudioThread));
    muxThreads = std::move(threadv);
}

void muxaudio_threads_stop()
{
    std::cout << "MUX stop all threads\n";
    //muxThreads[0].join();
    muxaudio_audio_process_stop();
}

} // end of Ms namespace

int main(int argc, char **argv)
{
    Ms::muxaudio_threads_start();
    while(1){
        std::this_thread::sleep_for(std::chrono::microseconds(100000));
    }
    return 0;
}


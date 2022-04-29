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
#include "driver.h"

namespace Ms {

static bool g_threads_started = false;

void mux_network_client_ctrl();
void mux_network_client_audio();

static std::vector<std::thread> muxThreads;

Driver* g_driver;
int g_ctrl_audio_error = 0;
int g_ctrl_audio_running = 0;

/*
void mux_teardown_driver (JackAudio *driver) {
    if (driver) {
        stopWait();
        delete driver;
        driver = 0;
    }
}
*/

void mux_ctrl_zmq_thread_init(std::string _notused)
{
    mux_network_client_ctrl();
}

void mux_audio_zmq_thread_init(std::string _notused)
{
    mux_network_client_audio();
}

void mux_threads_start()
{
    if (g_threads_started) return;
    g_threads_started = true;
    std::vector<std::thread> threadv;
    //
    std::thread zmqCtrlThread(mux_ctrl_zmq_thread_init, "notused");
    threadv.push_back(std::move(zmqCtrlThread));
    //
    std::thread zmqAudioThread(mux_audio_zmq_thread_init, "notused");
    threadv.push_back(std::move(zmqAudioThread));
    // move threads to heap
    muxThreads = std::move(threadv);
}

}

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

void mux_network_server_ctrl();
void mux_network_server_audio();
int mux_mq_to_audio_visit();
void mux_audio_process();

static std::vector<std::thread> muxThreads;
extern int mux_audio_process_run;

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

void mux_audio_control_thread_init(std::string _notused)
{
    while (1) {
        if (! mux_mq_to_audio_visit()) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
        }
    }
}

void mux_thread_process_init(std::string msg)
{
    std::cout << "MUX audio-process thread initialized:" << msg << "\n";
    mux_audio_process_run = 1;
    mux_audio_process();
}

void mux_ctrl_zmq_thread_init(std::string _notused)
{
    mux_network_server_ctrl();
}

void mux_audio_zmq_thread_init(std::string _notused)
{
    mux_network_server_audio();
}


void mux_threads_start()
{
    g_driver = driverFactory("");

    std::vector<std::thread> threadv;
    std::thread ctrlThread(mux_audio_control_thread_init, "notused");
    threadv.push_back(std::move(ctrlThread));
    std::thread procThread(mux_thread_process_init, "notused");
    threadv.push_back(std::move(procThread));
    std::thread zmqCtrlThread(mux_ctrl_zmq_thread_init, "notused");
    threadv.push_back(std::move(zmqCtrlThread));
    std::thread zmqAudioThread(mux_audio_zmq_thread_init, "notused");
    threadv.push_back(std::move(zmqAudioThread));
    muxThreads = std::move(threadv);
}

void mux_threads_stop()
{
    std::cout << "MUX stop all threads\n";
    mux_audio_process_run = 0;
    //muxThreads[0].join();
}

} // end of Ms namespace

int main(int argc, char **argv)
{
    Ms::mux_threads_start();
    while(1){
        std::this_thread::sleep_for(std::chrono::microseconds(100000));
    }
    return 0;
}


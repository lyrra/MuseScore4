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

void mux_network_server();
int mux_mq_to_audio_visit();

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

void mux_audio_control_thread_init(std::string _notused)
{
    while (1) {
        if (! mux_mq_to_audio_visit()) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
        }
    }
}

void mux_audio_zmq_thread_init(std::string _notused)
{
    mux_network_server();
}

void mux_threads_start()
{
    g_driver = driverFactory("");

    std::vector<std::thread> threadv;
    std::thread ctrlThread(mux_audio_control_thread_init, "notused");
    threadv.push_back(std::move(ctrlThread));
    std::thread zmqThread(mux_audio_zmq_thread_init, "notused");
    threadv.push_back(std::move(zmqThread));
    muxThreads = std::move(threadv);
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


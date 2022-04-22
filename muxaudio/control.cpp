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

int mux_mq_to_audio_visit();

static std::vector<std::thread> ctrlThreads;

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

void mux_control_start()
{
    std::vector<std::thread> threadv;
    std::thread procThread(mux_audio_control_thread_init, "notused");
    threadv.push_back(std::move(procThread));
    ctrlThreads = std::move(threadv);
}

}

int main(int argc, char **argv)
{
    return 0;
}


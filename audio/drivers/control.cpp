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
int g_ctrl_work = 0;
bool g_ctrl_hotPlug;

/*
void mux_teardown_driver (JackAudio *driver) {
    if (driver) {
        stopWait();
        delete driver;
        driver = 0;
    }
}
*/

int mux_audio_control_work(Driver* driver)
{
    switch (g_ctrl_work) {
        case 1: // hotPlug
            std::cout << "mux_audio_ctrl call driver::start\n";
            driver->start(g_ctrl_hotPlug);
            g_ctrl_audio_running = 1;
        break;
        //case X: // close driver, but keep control running
        //    mux_teardown_driver(driver);
        case -1: // exit
            return -1;
        break;
    }
    return -1;
}

void mux_audio_control_thread_init(std::string _notused)
{
    while (1) {
        int work_done = 0;
        if (g_ctrl_work) {
            work_done = 1;
            mux_audio_control_work(g_driver);
            g_ctrl_work = 0;
        }
        if (mux_mq_to_audio_visit()) {
            work_done = 1;
        }
        if (! work_done) {
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

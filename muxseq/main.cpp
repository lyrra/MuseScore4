/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <zmq.h>
#include <thread>
#include <chrono>
#include <cstring>
#include "mux.h"
namespace Ms {

#define MUX_SYNC_MSLEEP 100

extern bool g_state_play;

void muxseq_threads_start();


} // end of namespace Ms

int main(int argc, char **argv)
{
    fprintf(stderr, "muxseq initializing\n"); std::fflush(stderr);
    Ms::muxseq_threads_start();
    while(1){
        std::this_thread::sleep_for(std::chrono::microseconds(100000));
    }
    return 0;
}

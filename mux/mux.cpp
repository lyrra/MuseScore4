/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */
/*
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstring>
#include <zmq.h>

#include "mux.h"

namespace Mux {

void mux_zmq_ctrl_send_to_audio(int len, unsigned char *buf);

static std::vector<std::thread> seqThreads;
static int mux_audio_process_run = 0;

void mux_thread_process_init(std::string msg)
{
    // qDebug("MUX audio-process thread initialized.");
    //mux_audio_process();
}

void mux_start_threads()
{
    std::cout << "MUX start audio threads\n";
    //mux_audio_process_run = 1;
    std::vector<std::thread> threadv;
    std::thread procThread(mux_thread_process_init, "hi there!");
    threadv.push_back(std::move(procThread));
    seqThreads = std::move(threadv);
}

void mux_stop_threads()
{
    // qDebug("MUX stop audio threads");
    //mux_audio_process_run = 0;
    seqThreads[0].join();
}

/*** REQ/REP (query) client/server *****/

int mux_network_query_client (struct MuxSocket &sock, const char *url, bool req)
{
    std::cerr << "MUX ZeroMQ query network client connect " << url << "\n";
    sock.context = zmq_ctx_new();
    sock.socket = zmq_socket(sock.context, req ? ZMQ_REQ : ZMQ_REP);
    int rc = zmq_connect(sock.socket, url);
    if (rc) {
        fprintf(stderr, "zmq-bind error: %s\n", std::strerror(errno));
    }
    return rc;
}

int mux_network_query_server (struct MuxSocket &sock, const char* url, bool req)
{
    std::cerr << "MUX ZeroMQ query network server bind " << url << "\n";
    sock.context = zmq_ctx_new();
    sock.socket = zmq_socket(sock.context, req ? ZMQ_REQ : ZMQ_REP);
    int rc = zmq_bind(sock.socket, url);
    if (rc) {
        fprintf(stderr, "zmq-bind error: %s\n", std::strerror(errno));
    }
    return rc;
}

/*** PUB/SUB (bulletin) client/server *****/

int mux_network_bulletin_client (struct MuxSocket &sock, const char *url)
{
    std::cerr << "MUX ZeroMQ bulletin network client connect " << url << "\n";
    sock.context = zmq_ctx_new();
    sock.socket = zmq_socket(sock.context, ZMQ_SUB);
    int rc = zmq_connect(sock.socket, url);
    if (rc) {
        fprintf(stderr, "zmq-bind error: %s\n", std::strerror(errno));
    }
    return rc;
}

int mux_network_bulletin_server (struct MuxSocket &sock, const char* url)
{
    std::cerr << "MUX ZeroMQ bulletin network server bind " << url << "\n";
    sock.context = zmq_ctx_new();
    sock.socket = zmq_socket(sock.context, ZMQ_PUB);
    int rc = zmq_bind(sock.socket, url);
    if (rc) {
        fprintf(stderr, "zmq-bind error: %s\n", std::strerror(errno));
    }
    return rc;
}

/********/

void mux_network_close(struct MuxSocket &sock)
{
    zmq_close(sock.socket);
    zmq_ctx_destroy(sock.context);
}

#if 0
void mux_network_mainloop_ctrl()
{
    // qDebug("MUX ZeroMQ ctrl entering main-loop");
    while (1) {
        char msg[1024];
        if (zmq_recv(zmq_socket_ctrl, &msg, sizeof(msg), 0) < 0) {
            // qFatal("zmq-recv error: %s", std::strerror(errno));
            break;
        }
        // mux_mq_from_muxaudio_handle(msg);
    }
    // qDebug("mux_network_mainloop ctrl has exited");
}

void mux_zmq_ctrl_send_to_audio(int len, unsigned char *buf)
{
    // qDebug("zmq-send ctrl msg.type=%i (len %i)", msg.type, sizeof(struct Msg));
    zmq_send(zmq_socket_ctrl, buf, len, 0);
}

void mux_network_mainloop_audio()
{
    // qDebug("MUX ZeroMQ audio entering main-loop");
    while (1) {
        char c;
        if (zmq_recv(zmq_socket_audio, &c, 1, 0) < 0) {
            // qFatal("zmq-recv error: %s", std::strerror(errno));
            break;
        }
        // get MUX_CHUNKSIZE number of frames from the ringbuffer
        float frames[MUX_CHUNKSIZE]; // count stereo, ie number of floats needed
        //mux_process_bufferStereo(MUX_CHUNKSIZE, frames);

        zmq_send(zmq_socket_audio, frames, sizeof(float) * MUX_CHUNKSIZE, 0);
        //zmq_send(zmq_socket_audio, "b", 1, 0);
    }
    // qDebug("mux_network_mainloop audio has exited");
}
#endif

} // end of namespace Mux

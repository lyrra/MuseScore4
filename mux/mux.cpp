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

int mux_zmq_send (Mux::MuxSocket &muxsock, void* buf, int len) {
    return zmq_send(muxsock.socket, buf, len, 0);
}

int mux_zmq_recv (Mux::MuxSocket &muxsock, void* buf, int len) {
    return zmq_recv(muxsock.socket, buf, len, 0);
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

void mux_network_close(struct MuxSocket &sock)
{
    zmq_close(sock.socket);
    zmq_ctx_destroy(sock.context);
}

} // end of namespace Mux

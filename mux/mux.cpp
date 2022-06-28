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

#define LD(...) fprintf(stderr, __VA_ARGS__)
#define LE(...) { fprintf(stderr, __VA_ARGS__); }
#define LEX(...) { fprintf(stderr, __VA_ARGS__); exit(1); }

namespace Mux {

int mux_zmq_send (Mux::MuxSocket &muxsock, void* buf, int len) {
    return zmq_send(muxsock.socket, buf, len, 0);
}

int mux_zmq_recv (Mux::MuxSocket &muxsock, void* buf, int len) {
    return zmq_recv(muxsock.socket, buf, len, 0);
}

// different types of server/client pub/sub/req/rep combinations
// query req_or_pub server -- pub/sub sub part connect
// query REQ_OR_PUB server -- pub/sub pub part connect
// QUERY req_or_pub server -- QUERY   rep part connect
// QUERY REQ_OR_PUB server -- QUERY   req part connect
// query req_or_pub SERVER -- pub/sub sub part bind
// query REQ_OR_PUB SERVER -- pub/sub pub part bind
// QUERY req_or_pub SERVER -- QUERY   rep part bind
// QUERY REQ_OR_PUB SERVER -- QUERY   req part bind
int mux_make_connection(struct MuxSocket &sock, const char *url, ZmqType type, ZmqDir dir, ZmqServer server)
{
    const char *dirstr;
    switch (dir) {
      case ZmqDir::REQ: dirstr = "REQ"; break;
      case ZmqDir::REP: dirstr = "REP"; break;
      case ZmqDir::PUB: dirstr = "PUB"; break;
      case ZmqDir::SUB: dirstr = "SUB"; break;
      default : dirstr = "UNK";
    }
    LD("MUX ZeroMQ %s %s %s url %s\n",
       type == ZmqType::QUERY ? "query" : "pub/sub",
       dirstr,
       server == ZmqServer::BIND ? "bind" : "connect",
       url);
    sock.context = zmq_ctx_new();
    sock.socket = zmq_socket(sock.context,
                             dir == ZmqDir::REQ ? ZMQ_REQ :
                              (dir == ZmqDir::REP ? ZMQ_REP :
                               (dir == ZmqDir::PUB ? ZMQ_PUB :
                                ZMQ_SUB)));
    int rc;
    if (server == ZmqServer::BIND) {
        rc = zmq_bind(sock.socket, url);
    } else {
        rc = zmq_connect(sock.socket, url);
    }
    if (rc) {
        LD("zmq-%s-% error: %s\n", 
           type == ZmqType::QUERY ? "query" : "pub/sub",
           server == ZmqServer::BIND ? "bind" : "connect",
           std::strerror(errno));
    }
    return rc;
}

int mux_request (struct MuxSocket &sock, void* buf, int len) {
    if (zmq_send(sock.socket, buf, len, 0) < 0) return -1;
    if (zmq_recv(sock.socket, buf, len, 0) < 0) return -2;
    return 0;
}

int mux_query_send (struct MuxSocket &sock, void* buf, int len) {
    zmq_msg_t msg;
    int rc = zmq_msg_init_size(&msg, len);
    if (rc < 0) return rc;
    memcpy(zmq_msg_data(&msg), buf, len);
    rc = zmq_msg_send(&msg, sock.socket, 0);
    zmq_msg_close (&msg);
    return (rc == len ? len : -1);
}

void* mux_query_recv (struct MuxSocket &sock, int *rlen) {
    int more;
    size_t more_size = sizeof (more);
    int len = 0;
    void* m = nullptr;
    do {
        zmq_msg_t msg;
        int rc = zmq_msg_init(&msg);
        if (rc < 0) {
            if (m) free(m);
            return nullptr;
        }
        rc = zmq_msg_recv (&msg, sock.socket, 0);
        if (rc < 0) {
            if (m) free(m);
            return nullptr;
        }
        int gotlen = rc;
        rc = zmq_getsockopt (sock.socket, ZMQ_RCVMORE, &more, &more_size);
        if (rc < 0) {
            if (m) free(m);
            return nullptr;
        }
        int newlen = len + gotlen;
        void* tm = malloc(newlen);
        if (len > 0) { // copy previous data
            memcpy(tm, m, len);
            free(m);
        }
        m = tm;
        memcpy((unsigned char*)m+len, zmq_msg_data(&msg), gotlen);
        len = newlen;
        zmq_msg_close (&msg);
    } while (more);
    *rlen = len;
    return m;
}

void mux_network_close(struct MuxSocket &sock)
{
    zmq_close(sock.socket);
    zmq_ctx_destroy(sock.context);
}

} // end of namespace Mux

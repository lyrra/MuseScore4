/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#ifndef __MUX_H__
#define __MUX_H__

namespace Mux {

enum class ZmqType {
  QUERY = 0,
  PUBSUB
};
enum class ZmqDir {
  REQ = 0,
  REP,
  PUB,
  SUB
};
enum class ZmqServer {
  BIND = 0,
  CONNECT
};

struct MuxSocket {
    void *context;
    void *socket;
};

int mux_zmq_send (struct MuxSocket &muxsock, void* buf, int len);
int mux_zmq_recv (MuxSocket &muxsock, void* buf, int len);
int mux_query_send (struct MuxSocket &sock, void* buf, int len);
void* mux_query_recv (struct MuxSocket &sock, int *rlen);
int mux_request (struct MuxSocket &sock, void* buf, int len);
int mux_make_connection(struct MuxSocket &sock, const char *url, ZmqType type, ZmqDir dir, ZmqServer server);


} // namespace Mux
#endif

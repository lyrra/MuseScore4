/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#ifndef __MUX_H__
#define __MUX_H__

namespace Mux {

struct MuxSocket {
    void *context;
    void *socket;
};

int mux_network_server (struct MuxSocket &sock, const char* url);
int mux_network_connect (struct MuxSocket &sock, const char *url);

} // namespace Mux
#endif

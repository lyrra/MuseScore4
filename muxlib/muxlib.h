#ifndef __MUXLIB_H__
#define __MUXLIB_H__

#define MUX_MUSESCORE_QUERY_CLIENT_URL "tcp://localhost:7701"
#define MUX_MUSESCORE_BULLETIN_CLIENT_URL "tcp://localhost:7702"

namespace Ms {

void mux_musescore_client_start();

} // namespace Ms
#endif

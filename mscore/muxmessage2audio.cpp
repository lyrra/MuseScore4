/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#include "seq.h"

namespace Ms {

void msgToAudioSeekTransport(int utick) {
    struct Msg msg;
    msg.type = MsgTypeTransportSeek;
    msg.payload.i = utick;
    mux_mq_to_audio_writer_put(msg);
}

}

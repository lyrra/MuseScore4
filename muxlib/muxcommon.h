/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#ifndef __MUXCOMMON_H__
#define __MUXCOMMON_H__

namespace Ms {

// transport states (also the jack audio transport states)
enum class Transport : char {
    STOP=0,
    PLAY=1,
    STARTING=3,
    NET_STARTING=4
};

void msgToAudioSeekTransport(int utick);

} // namespace Ms
#endif

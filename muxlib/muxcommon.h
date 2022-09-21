/* GPL-2.0-or-later
 * Copyright (C) 2022 Larry Valkama
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. 
 */

#ifndef __MUXCOMMON_H__
#define __MUXCOMMON_H__

#define LL(str) { snprintf(Ms::_logbuf, 256, str        ); Ms::_log_write(); }
#define LE(...) { snprintf(Ms::_logbuf, 256, __VA_ARGS__); Ms::_log_write(); }
#define LW(...) { snprintf(Ms::_logbuf, 256, __VA_ARGS__); Ms::_log_write(); }
//#ifdef LOGLEVEL
#define LD(...) { snprintf(Ms::_logbuf, 256, __VA_ARGS__); Ms::_log_write(); }
#define LD2(...) { snprintf(Ms::_logbuf, 256, __VA_ARGS__); Ms::_log_write(); }
#define LD4(...) { snprintf(Ms::_logbuf, 256, __VA_ARGS__); Ms::_log_write(); }
#define LD6(...) { snprintf(Ms::_logbuf, 256, __VA_ARGS__); Ms::_log_write(); }
#define LD8(...) { snprintf(Ms::_logbuf, 256, __VA_ARGS__); Ms::_log_write(); }
//#else
//#define LD(...)  (void)0
//#define LD2(...) (void)0
//#define LD4(...) (void)0
//#define LD6(...) (void)0
//#define LD8(...) (void)0
//#endif

namespace Ms {

extern void (*g_logstr) (char *str);
extern thread_local char _logbuf[256];
void _log_write ();

// transport states (also the jack audio transport states)
enum class Transport : char {
    STOP=0,
    PLAY=1,
    STARTING=3,
    NET_STARTING=4
};



} // namespace Ms
#endif

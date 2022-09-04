//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2012 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#if (defined (_MSCVER) || defined (_MSC_VER))
// Include stdint.h and #define _STDINT_H to prevent <systemdeps.h> from redefining types
// #undef UNICODE to force LoadLibrary to use the char-based implementation instead of the wchar_t one.
#include <stdint.h> // ??? config.h should take care of this
#include <thread>
#define _STDINT_H 1  
#endif

#include "config.h"
#include "driver.h"

//#include "mscore/preferences.h"

#ifdef USE_JACK
#include "jackaudio.h"
#endif

#ifdef USE_PORTAUDIO
#include "pa.h"
#endif

namespace Ms {

struct {
      bool PREF_IO_JACK_USEJACKAUDIO = true;
      bool PREF_IO_JACK_USEJACKMIDI = true;
      bool PREF_IO_ALSA_USEALSAAUDIO = false;
      bool PREF_IO_PORTAUDIO_USEPORTAUDIO = false;
      bool PREF_IO_PULSEAUDIO_USEPULSEAUDIO = false;
} preferences;

//#ifdef USE_PULSEAUDIO
//extern Driver* getPulseAudioDriver();
//#endif

bool alsaIsUsed = false, jackIsUsed = false, portAudioIsUsed = false, pulseAudioIsUsed = false;


//---------------------------------------------------------
//   driverFactory
//    driver can be: jack alsa pulse portaudio
//---------------------------------------------------------

Driver* driverFactory(std::string driverName)
      {
      Driver* driver = 0;

    if (driverName.empty()) {
        return nullptr;
    }
    //FIX: driverName = lowercase(driverName);

    bool useJackFlag       = false;
    bool useAlsaFlag       = false;
    bool usePortaudioFlag  = false;
    bool usePulseAudioFlag = false;

    if (driverName == "jack") {
        useJackFlag = true;
        preferences.PREF_IO_JACK_USEJACKAUDIO = true;
        preferences.PREF_IO_JACK_USEJACKMIDI = true;
    } else if (driverName == "alsa") {
        useAlsaFlag = true;
        preferences.PREF_IO_ALSA_USEALSAAUDIO = true;
    } else if (driverName == "pulse") {
        usePulseAudioFlag = true;
        preferences.PREF_IO_PULSEAUDIO_USEPULSEAUDIO = true;
    } else if (driverName == "portaudio") {
        usePortaudioFlag = true;
        preferences.PREF_IO_PORTAUDIO_USEPORTAUDIO = true;
    }

/*
    if (usePulseAudioFlag) {
        driver = getPulseAudioDriver();
        if (!driver->init()) {
            qDebug("init PulseAudio failed");
            delete driver;
            driver = 0;
        } else {
            pulseAudioIsUsed = true;
        }
    }
*/
    if (usePortaudioFlag) {
        driver = new Portaudio();
        if (!driver->init()) {
            qDebug("init PortAudio failed");
            delete driver;
            driver = 0;
        } else {
            portAudioIsUsed = true;
        }
    }
/*
    if (driver == 0 && useAlsaFlag) {
        driver = new AlsaAudio();
        if (!driver->init()) {
            qDebug("init ALSA driver failed");
            delete driver;
            driver = 0;
        } else {
            alsaIsUsed = true;
        }
    }
*/
    if (useJackFlag) {
        useAlsaFlag      = false;
        usePortaudioFlag = false;
        driver = new JackAudio();
        if (!driver->init()) {
            qDebug("no JACK server found");
            delete driver;
            driver = 0;
        } else {
            jackIsUsed = true;
        }
    } else {
        qDebug("WARNING: JACK (https://jackaudio.org/) is not used (superior)");
    }

    if (driver == 0)
        qDebug("no audio driver found");

    return driver;
}

}


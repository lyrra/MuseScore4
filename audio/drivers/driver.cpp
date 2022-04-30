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

#include "config.h"
#include "driver.h"

namespace Ms {

void mux_threads_start();

//---------------------------------------------------------
//   driverFactory
//    driver can be: jack alsa pulse portaudio
//---------------------------------------------------------

Driver* driverFactory(Seq* seq, QString driverName)
      {
      Driver* driver = 0;
      if (!driverName.isEmpty()) {
            driverName        = driverName.toLower();
            }
      if (driver == 0)
            qDebug("no audio driver found");

      mux_threads_start();

      return driver;
      }

}


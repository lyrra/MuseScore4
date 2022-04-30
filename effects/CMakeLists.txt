#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2013 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

QT5_ADD_RESOURCES (qrc_effects_files
     zita1/zita.qrc
     )

include_directories (${QT_INCLUDES})

add_library (effects STATIC
      ${_all_h_file}
      ${qrc_effects_files}
      effect.cpp
      noeffect/noeffect.cpp
      zita1/zita.cpp
      compressor/compressor.cpp
      )

set_target_properties (
      effects
      PROPERTIES
         COMPILE_FLAGS "-g -Wall -Wextra ${PCH_INCLUDE} -Winvalid-pch"
      )

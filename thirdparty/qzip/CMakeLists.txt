#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2014 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

include_directories (${QT_INCLUDES})

add_library(qzip STATIC
   ${_all_h_file}
   qzip.cpp
   )

set_target_properties (
   qzip
   PROPERTIES
      COMPILE_FLAGS "-g -Wall -Wextra ${PCH_INCLUDE} -Winvalid-pch"
   )

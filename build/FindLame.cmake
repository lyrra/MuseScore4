find_path(LAME_INCLUDE_DIR lame/lame.h)

#find_library(LAME_LIBRARY NAMES mp3lame lame_enc)
find_library(LAME_LIBRARY NAMES mp3lame)

if (LAME_INCLUDE_DIR AND LAME_LIBRARY)
      set(LAME_FOUND TRUE)
endif (LAME_INCLUDE_DIR AND LAME_LIBRARY)

if (LAME_FOUND)
      if (NOT LAME_FIND_QUIETLY)
            message (STATUS "Found lame: ${LAME_LIBRARY}")
      endif (NOT LAME_FIND_QUIETLY)
else (LAME_FOUND)
      if (LAME_FIND_REQUIRED)
            message (FATAL_ERROR "Could not find: lame")
      endif (LAME_FIND_REQUIRED)
endif (LAME_FOUND)

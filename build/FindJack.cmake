find_path(JACK_INCLUDE_DIR jack/jack.h)

if (MINGW)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a")
else (MINGW)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif (MINGW)

find_library(JACK_LIBRARY NAMES libjack64)


if (JACK_INCLUDE_DIR AND JACK_LIBRARY)
    set(JACK_FOUND TRUE)
    message(STATUS "JACK INC: ${JACK_INCLUDE_DIR}")
    message(STATUS "JACK LIB: ${JACK_LIBRARY}")
else (JACK_INCLUDE_DIR AND JACK_LIBRARY)
    message (FATAL_ERROR "Could not find: Jack")
endif (JACK_INCLUDE_DIR AND JACK_LIBRARY)

find_path(SNDFILE_INCLUDE_DIR sndfile.h)

if (MINGW)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a")
else (MINGW)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif (MINGW)
find_library(SNDFILE_LIBRARY NAMES sndfile libsndfile-1)

message(STATUS "SNDFILE INC: ${SNDFILE_INCLUDE_DIR}")
message(STATUS "SNDFILE LIB: ${SNDFILE_LIBRARY}")

if (SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)
      set(SNDFILE_FOUND TRUE)
endif (SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)

if (SNDFILE_FOUND)
      message (STATUS "Found sndfile: ${SNDFILE_LIBRARY}")
else (SNDFILE_FOUND)
      message (FATAL_ERROR "Could not find: sndfile")
endif (SNDFILE_FOUND)

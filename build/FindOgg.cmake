find_path(OGG_INCLUDE_DIR ogg/ogg.h)

if (MINGW)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a")
else (MINGW)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif (MINGW)
find_library(OGG_LIBRARY NAMES libogg)

message(STATUS "OGG INC: ${OGG_INCLUDE_DIR}")
message(STATUS "OGG LIB: ${OGG_LIBRARY}")

if (OGG_INCLUDE_DIR AND OGG_LIBRARY)
      set(OGG_FOUND TRUE)
endif (OGG_INCLUDE_DIR AND OGG_LIBRARY)

if (OGG_FOUND)
      message (STATUS "Found ogg: ${OGG_LIBRARY}")
else (OGG_FOUND)
      message (FATAL_ERROR "Could not find: ogg")
endif (OGG_FOUND)

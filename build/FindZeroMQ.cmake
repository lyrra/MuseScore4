
find_path(ZMQ_INCLUDE_DIR zmq.h)

if (MINGW)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a")
else (MINGW)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif (MINGW)
find_library(ZMQ_LIBRARY NAMES zmq)

message(STATUS ${ZMQ_LIBRARY})

if (ZMQ_INCLUDE_DIR AND ZMQ_LIBRARY)
      set(ZMQ_FOUND TRUE)
endif (ZMQ_INCLUDE_DIR AND ZMQ_LIBRARY)

if (ZMQ_FOUND)
      message (STATUS "Found ZeroMQ: ${ZMQ_LIBRARY}")
else (ZMQ_FOUND)
      message (FATAL_ERROR "Could not find: zmq")
endif (ZMQ_FOUND)

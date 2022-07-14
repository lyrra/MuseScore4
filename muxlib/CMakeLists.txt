
set(MODULE muxlib)

include (${PROJECT_SOURCE_DIR}/build/gch.cmake)

if (APPLE)
      file(GLOB_RECURSE INCS "*.h")
else (APPLE)
      set(INCS "")
endif (APPLE)

if (NOT MSVC)
   set(_all_h_file "${PROJECT_BINARY_DIR}/all.h")
else (NOT MSVC)
   set(_all_h_file "${PROJECT_SOURCE_DIR}/all.h")
endif (NOT MSVC)

include_directories (
      ${QT_INCLUDES}
      ${PROJECT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_LIST_DIR}
      ${PROJECT_SOURCE_DIR}/mux
      ${PROJECT_SOURCE_DIR}/mscore
      ${PROJECT_SOURCE_DIR}/muxseq
      ${PROJECT_SOURCE_DIR}/thirdparty  # because include "mscore/musescore.h"
      )

add_library (${MODULE} STATIC
      ${_all_h_file}
      ${PCH}
      midi/midipatch.h
      event.h event.cpp
      muxseq_client.h muxseq_client.cpp
      muxseqsig.h muxseqsig.cpp
      muxseqlib.h muxseqlib.cpp
      muxlib.h muxlib.cpp
      )

target_link_libraries (${MODULE}
      mux
      Qt5::Core)

if (NOT MSVC)
   set_target_properties (
      ${MODULE}
      PROPERTIES
         COMPILE_FLAGS "${PCH_INCLUDE} -g -Wall -Wextra -Winvalid-pch"
      )
else (NOT MSVC)
   set_target_properties (
      ${MODULE}
      PROPERTIES
         COMPILE_FLAGS "${PCH_INCLUDE}"
      )
endif (NOT MSVC)   

# xcode_pch(${MODULE} all)

# # Use MSVC pre-compiled headers
# vstudio_pch( ${MODULE} )

# MSVC does not depend on mops1 & mops2 for PCH
if (NOT MSVC)
   ADD_DEPENDENCIES(${MODULE} mops1)
   ADD_DEPENDENCIES(${MODULE} mops2)
endif (NOT MSVC)   



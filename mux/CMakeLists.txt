# GPL-2.0-or-later

#include (${PROJECT_SOURCE_DIR}/build/gch.cmake)

#if (APPLE)
#      file(GLOB_RECURSE INCS "*.h")
#else (APPLE)
#      set(INCS "")
#endif (APPLE)

#if (NOT MSVC)
#   set(_all_h_file "${PROJECT_BINARY_DIR}/all.h")
#else (NOT MSVC)
#   set(_all_h_file "${PROJECT_SOURCE_DIR}/all.h")
#endif (NOT MSVC)

#include_directories (
#      ${PROJECT_SOURCE_DIR}
#      ${CMAKE_CURRENT_BINARY_DIR}
#      ${CMAKE_CURRENT_LIST_DIR}
#      )

add_library(mux STATIC
      #${_all_h_file}
      #${PCH}
      mux.h mux.cpp
      muxqueue.h muxqueue.cpp
      )

#target_link_libraries (${MODULE}
#      ${QT_LIBRARIES}
#      )

#if (NOT MSVC)
#   set_target_properties (
#      ${MODULE}
#      PROPERTIES
#         COMPILE_FLAGS "${PCH_INCLUDE} -g -Wall -Wextra -Winvalid-pch"
#      )
#else (NOT MSVC)
#   set_target_properties (
#      ${MODULE}
#      PROPERTIES
#         COMPILE_FLAGS "${PCH_INCLUDE}"
#      )
#endif (NOT MSVC)   

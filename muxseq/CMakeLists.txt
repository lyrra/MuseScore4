# GPL-2.0-or-later

include_directories(
      ${PROJECT_SOURCE_DIR}/global
      ${QTSINGLEAPPLICATION_INCLUDE_DIRS}
      )

add_executable(muxseq
      main.cpp
      control.cpp
      mux.h mux.cpp
      seq.h seq.cpp
      muxseq.h muxseq.cpp
      )

target_include_directories(muxseq PRIVATE ${PROJECT_SOURCE_DIR}
                          )

target_link_libraries(muxseq ${ZMQ_LIBRARY})

if (MINGW)
   string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
   set_target_properties(muxseq
      PROPERTIES
         COMPILE_FLAGS "${PCH_INCLUDE} -g -Wall -Wextra -Winvalid-pch ${QT_DEFINITIONS} -DQT_CORE_LIB"
         LINK_FLAGS "-L ${QT_INSTALL_LIBS}"
      )

   # Keep dependencies in alphabetical order. Changes made to this list
   # might need to be made in "build/Linux+BSD/portable/copy-libs" too.
   get_filename_component(COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
   get_filename_component (MINGW_ROOT ${COMPILER_DIR} DIRECTORY)

   target_link_libraries(muxseq ${QT_LIBRARIES})

   install( TARGETS muxseq RUNTIME DESTINATION bin )

   if (BUILD_64)
      install( FILES
         ${MINGW_ROOT}/bin/libgcc_s_seh-1.dll
         DESTINATION bin)
   else (BUILD_64)
      install( FILES
         ${MINGW_ROOT}/bin/libgcc_s_dw2-1.dll
         DESTINATION bin)
   endif (BUILD_64)

   install( FILES
      ${MINGW_ROOT}/bin/libstdc++-6.dll
      ${MINGW_ROOT}/bin/libwinpthread-1.dll
      DESTINATION bin)

else (MINGW)

   if ( NOT MSVC )
      set_target_properties (
         muxseq
         PROPERTIES
            COMPILE_FLAGS "${PCH_INCLUDE} -g -Wall -Wno-overloaded-virtual -Winvalid-pch"
         )

      if (APPLE)
        target_link_libraries(seq ${OsxFrameworks})
      else (APPLE)
          target_link_libraries(seq rt)
      endif (APPLE)

      if (APPLE)
         set_target_properties(seq
           PROPERTIES
              LINK_FLAGS "-stdlib=libc++"
           )
        xcode_pch(muxseq all)
        install (TARGETS muxseq BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX})
      else (APPLE)
        #### PACKAGING for Linux and BSD based systems (more in top-level CMakeLists.txt) ####
        # Install executable
        install( TARGETS muxseq RUNTIME DESTINATION bin )
      endif (APPLE)
   else ( NOT MSVC )
      # Microsoft Visual Studio-specific starts here!
      string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)

      # Create list of directories to search for libraries
      foreach (item ${CMAKE_LIBRARY_PATH})
         string( APPEND all_library_paths " /LIBPATH:${item}" )
      endforeach()

      # Windows: Add /SUBSYSTEM:WINDOWS to LINK_FLAGS to avoid a console window in release
      if(CMAKE_BUILD_TYPE MATCHES "REL*")
        set_target_properties(muxseq
           PROPERTIES
              COMPILE_FLAGS "${PCH_INCLUDE}"
              LINK_FLAGS    "/LIBPATH:${QT_INSTALL_LIBS} ${all_library_paths}"
              LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
           )
      else(CMAKE_BUILD_TYPE MATCHES "REL*")
        set_target_properties(muxseq
           PROPERTIES
              COMPILE_FLAGS "${PCH_INCLUDE}"
              LINK_FLAGS    "/LIBPATH:${QT_INSTALL_LIBS} ${all_library_paths}"
              LINK_FLAGS "/SUBSYSTEM:CONSOLE"
           )
      endif(CMAKE_BUILD_TYPE MATCHES "REL*")

      set(CMAKE_FIND_LIBRARY_PREFIX "")
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll")

      # Keep dependencies in alphabetical order. Changes made to this list
      # might need to be made in "build/Linux+BSD/portable/copy-libs" too.
      get_filename_component(COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
      get_filename_component (MINGW_ROOT ${COMPILER_DIR} DIRECTORY)

      install(TARGETS muxseq RUNTIME DESTINATION bin) # this duplicate due to the correctly package step

   endif ( NOT MSVC )
endif (MINGW)


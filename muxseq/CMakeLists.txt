# GPL-2.0-or-later

include_directories(
      ${PROJECT_SOURCE_DIR}/global
      ${PROJECT_SOURCE_DIR}/mscore
      ${PROJECT_SOURCE_DIR}/thirdparty
      ${PROJECT_SOURCE_DIR}/muxlib
      ${PROJECT_SOURCE_DIR}/muxtools
      ${QTSINGLEAPPLICATION_INCLUDE_DIRS}
      )
set (FLUID_DIR ${CMAKE_CURRENT_LIST_DIR}/fluid)
set (FLUID_SRC
    ${FLUID_DIR}/fluid.cpp
    ${FLUID_DIR}/fluid.h
    ${FLUID_DIR}/chan.cpp
    ${FLUID_DIR}/conv.cpp
    ${FLUID_DIR}/conv.h
    ${FLUID_DIR}/dsp.cpp
    ${FLUID_DIR}/gen.cpp
    ${FLUID_DIR}/gen.h
    ${FLUID_DIR}/mod.cpp
    ${FLUID_DIR}/sfont.cpp
    ${FLUID_DIR}/sfont.h
    ${FLUID_DIR}/sfont3.cpp
    ${FLUID_DIR}/voice.cpp
    ${FLUID_DIR}/voice.h
    )
#set (FLUID_UI
#    ${FLUID_DIR}/fluid_gui.ui
#    )
set (ZERBERUS_SRC )
#set (ZERBERUS_UI )
if (ZERBERUS)

    set (ZERBERUS_DIR ${CMAKE_CURRENT_LIST_DIR}/zerberus)
    set (ZERBERUS_SRC
        ${ZERBERUS_DIR}/zerberus.cpp
        ${ZERBERUS_DIR}/zerberus.h
        ${ZERBERUS_DIR}/channel.cpp
        ${ZERBERUS_DIR}/channel.h
        ${ZERBERUS_DIR}/filter.cpp
        ${ZERBERUS_DIR}/filter.h
        ${ZERBERUS_DIR}/instrument.cpp
        ${ZERBERUS_DIR}/instrument.h
        ${ZERBERUS_DIR}/sample.h
        ${ZERBERUS_DIR}/sfz.cpp
        ${ZERBERUS_DIR}/voice.cpp
        ${ZERBERUS_DIR}/voice.h
        ${ZERBERUS_DIR}/zone.cpp
        ${ZERBERUS_DIR}/zone.h
        )

#    set (ZERBERUS_UI
#        ${ZERBERUS_DIR}/zerberus_gui.ui
#        )

endif (ZERBERUS)

add_executable(muxseq
      main.cpp
      control.cpp
      mux.h mux.cpp
      seq.h seq.cpp
      muxseq_api.h muxseq_api.cpp
      msynthesizer.h msynthesizer.cpp
      ${FLUID_SRC}
      ${ZERBERUS_SRC}
      )

target_include_directories(muxseq PRIVATE ${PROJECT_SOURCE_DIR}
                          )

target_link_libraries(muxseq
    effects
    qzip
    audiofile
    z
    ${ZMQ_LIBRARY}
    )

if (MINGW)
   string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
   set_target_properties(muxseq
      PROPERTIES
         COMPILE_FLAGS "${PCH_INCLUDE} -g -Wall -Wextra -Winvalid-pch ${QT_DEFINITIONS} -DQT_CORE_LIB"
         LINK_FLAGS "-L ${QT_INSTALL_LIBS} -mconsole"
         #LINK_FLAGS "-L ${QT_INSTALL_LIBS} -municode -mconsole"
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


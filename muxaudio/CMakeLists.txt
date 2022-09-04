# GPL-2.0-or-later

CMAKE_MINIMUM_REQUIRED(VERSION 3.3.0)

SET(JACK_LONGNAME "JACK (Jack Audio Connection Kit)")
SET(JACK_MIN_VERSION "0.98.0")

option(BUILD_JACK    "Build with support for ${JACK_LONGNAME} audio backend. JACK >= ${JACK_MIN_VERSION} will be needed." ON)
option(BUILD_PULSEAUDIO "Build with support for PulseAudio audio backend." OFF)
option(BUILD_ALSA "Build with support for ALSA audio backend." OFF)
option(BUILD_PORTAUDIO "Build with support for PortAudio audio backend." ON)
option(BUILD_PORTMIDI "Build with support for PortAudio's MIDI features." ${BUILD_PORTAUDIO}) # PortAudio required

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(PkgConfig)
find_package(Threads REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)


# Disable components not supported on Windows
if (MSVC OR MINGW)
    set(WIN_NOT_AVAIL "Not available on Windows")
    option(BUILD_PULSEAUDIO ${WIN_NOT_AVAIL} OFF)
    option(BUILD_ALSA ${WIN_NOT_AVAIL} OFF)
endif (MSVC OR MINGW)

# Disable components not supported on Mac
if (APPLE)
      set(MAC_NOT_AVAIL "Not available on Mac")
      option(BUILD_PULSEAUDIO ${MAC_NOT_AVAIL} OFF)
      option(BUILD_ALSA ${MAC_NOT_AVAIL} OFF)
endif (APPLE)

# Disable components not supported on Linux/BSD
if (NOT APPLE AND NOT MINGW AND NOT MSVC)
      set(NIX_NOT_AVAIL "Not available on Linux/BSD")
      #option(BUILD_PORTMIDI "PortMidi disabled on Linux. (It uses ALSA but it's better to use ALSA directly)" OFF)
endif (NOT APPLE AND NOT MINGW AND NOT MSVC)

##
## Find JACK >= JACK_MIN_VERSION
##
IF(BUILD_JACK)
    set ( USE_JACK 1 )
    pkg_check_modules(JACK "jack")
ENDIF(BUILD_JACK)

##
## PortAudio
##

if (BUILD_PORTAUDIO)
    set ( USE_PORTAUDIO 1 )
    include(FindPortAudio)
else (BUILD_PORTAUDIO)
     message(STATUS "PortAudio support disabled")
endif (BUILD_PORTAUDIO)

##
## PortMidi
##

if (BUILD_PORTMIDI)
    if (NOT BUILD_PORTAUDIO)
        message(SEND_ERROR "Error: PortMidi support requested (BUILD_PORTMIDI=${BUILD_PORTMIDI}), but PortAudio support is disabled (BUILD_PORTAUDIO=${BUILD_PORTAUDIO}). PortMidi requires PortAudio.")
    endif (NOT BUILD_PORTAUDIO)
    pkg_check_modules(PORTMIDI "portmidi")
    set (USE_PORTMIDI 1)
else (BUILD_PORTMIDI)
    message(STATUS "PortMidi support disabled")
endif (BUILD_PORTMIDI)

##
## ALSA >= 1.0.0
##

#if (BUILD_ALSA)
#      PKGCONFIG1 (alsa 1.0.0 ALSA_INCDIR ALSA_LIBDIR ALSA_LIB ALSA_CPP )
#      if (NOT ALSA_INCDIR)
#            message(SEND_ERROR "Error: ALSA support requested (BUILD_ALSA=${BUILD_ALSA}), but ALSA >= 1.0.0 was not found.")
#      else (NOT ALSA_INCDIR)
#            message("ALSA >= 1.0.0 found. ALSA support enabled. INCDIR ${ALSA_INCDIR}, LIBDIR ${ALSA_LIBDIR}, LIB ${ALSA_LIB}, CPP ${ALSA_CPP}")
#            set (USE_ALSA 1)
#      endif (NOT ALSA_INCDIR)
#else (BUILD_ALSA)
#     message(STATUS "ALSA support disabled")
#endif (BUILD_ALSA)

##
## MIDI
##

if (APPLE OR MINGW OR MSVC)
      set (HAS_MIDI 1)
else (APPLE OR MINGW OR MSVC)
      if (USE_ALSA)
            set (HAS_MIDI 1)
      endif (USE_ALSA)
endif (APPLE OR MINGW OR MSVC)

##
## PulseAudio
##

if (BUILD_PULSEAUDIO)
      include (FindPulseAudio)
      if (PULSEAUDIO_FOUND)
            set(USE_PULSEAUDIO 1)
            message("PulseAudio found. PulseAudio support enabled.")
      else (PULSEAUDIO_FOUND)
            message(SEND_ERROR "Error: PulseAudio support requested (BUILD_PULSEAUDIO=${BUILD_PULSEAUDIO}), but PulseAudio was not found.")
      endif (PULSEAUDIO_FOUND)
else (BUILD_PULSEAUDIO)
      message(STATUS "PulseAudio support disabled")
endif (BUILD_PULSEAUDIO)



include_directories(
      ${PROJECT_SOURCE_DIR}/global
      ${PROJECT_SOURCE_DIR}/mux
      ${PROJECT_SOURCE_DIR}/muxlib
      ${PROJECT_SOURCE_DIR}/muxtools # needed for midifile.h
      ${PROJECT_SOURCE_DIR}/muxseq # FIX: wrong (needs mux.h)
      ${PROJECT_SOURCE_DIR}/muxaudio/driver
      ${QT_INCLUDES}
      )

if (USE_JACK)
    add_compile_definitions(USE_JACK)
    set(JACK_SOURCE "driver/jackaudio.h" "driver/jackaudio.cpp")
    if (MINGW)
        set(JACK_SOURCE "${JACK_SOURCE}" "driver/jackweakapi.cpp")
    endif (MINGW)
endif (USE_JACK)

if (USE_PORTAUDIO)
    add_compile_definitions(USE_PORTAUDIO)
    set(PA_SOURCE "driver/pa.h" "driver/pa.cpp")
endif (USE_PORTAUDIO)

if (USE_PULSEAUDIO)
    add_compile_definitions(USE_PULSEAUDIO)
    set(PM_SOURCE "driver/pm.h" "driver/pm.cpp")
endif (USE_PULSEAUDIO)

add_executable(muxaudio
      muxaudio.h muxaudio.cpp
      control.h control.cpp
      driver/mididriver.h driver/mididriver.cpp
      driver/driver.h driver/driver.cpp
      ${JACK_SOURCE}
      ${PA_SOURCE}
      ${PM_SOURCE}
      )

target_include_directories(muxaudio PRIVATE ${PROJECT_SOURCE_DIR}
                                    PUBLIC ${PORTAUDIO_INCLUDE_DIRS}
                                           ${PORTMIDI_INCLUDE_DIR}
                                           ${JACK_INCLUDE_DIRS}
                                           ${ZMQ_INCLUDE_DIRS}
                          )


# APPLE: target_link_libraries ${OsxFrameworks}
target_link_libraries(muxaudio
    PRIVATE Threads::Threads
            mux
            muxlib
            ${ZMQ_LIBRARIES}
            ${PORTAUDIO_LIBRARIES} ${PORTAUDIO_LIBRARY} ${PORTMIDI_LIBRARY}
            ${JACK_LIBRARIES}
            Qt5::Core
            )

if (MINGW)
   string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
   set_target_properties( muxaudio
      PROPERTIES
         COMPILE_FLAGS "${PCH_INCLUDE} -g -Wall -Wextra -Winvalid-pch ${QT_DEFINITIONS} -DQT_CORE_LIB"
         LINK_FLAGS "-L ${QT_INSTALL_LIBS}"
      )

   # Keep dependencies in alphabetical order. Changes made to this list
   # might need to be made in "build/Linux+BSD/portable/copy-libs" too.
   get_filename_component(COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
   get_filename_component (MINGW_ROOT ${COMPILER_DIR} DIRECTORY)

   install( TARGETS muxaudio RUNTIME DESTINATION bin )

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
      ${OGG_LIBRARY}
      ${SNDFILE_LIBRARY}
      ${VORBIS_LIBRARY}
      ${VORBISFILE_LIBRARY}
      ${PORTAUDIO_LIBRARIES}
      ${PORTMIDI_LIBRARIES}
      ${JACK_LIBRARIES}
      DESTINATION bin)

else (MINGW)

   if ( NOT MSVC )
      set_target_properties (
         muxaudio
         PROPERTIES
            COMPILE_FLAGS "${PCH_INCLUDE} -g -Wall -Wno-overloaded-virtual -Winvalid-pch"
         )

      if (APPLE)
         set_target_properties(muxaudio
           PROPERTIES
              LINK_FLAGS "-stdlib=libc++"
           )
        xcode_pch(muxaudio all)
        install (TARGETS muxaudio BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX})
      else (APPLE)
        #### PACKAGING for Linux and BSD based systems (more in top-level CMakeLists.txt) ####
        # Install executable
        install( TARGETS muxaudio RUNTIME DESTINATION bin )
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
        set_target_properties( muxaudio
           PROPERTIES
              COMPILE_FLAGS "${PCH_INCLUDE}"
              LINK_FLAGS    "/LIBPATH:${QT_INSTALL_LIBS} ${all_library_paths}"
              LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
           )
      else(CMAKE_BUILD_TYPE MATCHES "REL*")
        set_target_properties( muxaudio
           PROPERTIES
              COMPILE_FLAGS "${PCH_INCLUDE}"
              LINK_FLAGS    "/LIBPATH:${QT_INSTALL_LIBS} ${all_library_paths}"
              LINK_FLAGS "/SUBSYSTEM:CONSOLE"
           )
      endif(CMAKE_BUILD_TYPE MATCHES "REL*")

      set(CMAKE_FIND_LIBRARY_PREFIX "")
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll")


      find_library( dll_lame        NAMES "lame_enc"       PATHS ${DEPENDENCIES_DIR} NO_DEFAULT_PATH)
      find_library( dll_ogg         NAMES "libogg"         PATHS ${DEPENDENCIES_DIR} NO_DEFAULT_PATH)
      find_library( dll_sndfile     NAMES "sndfile" "libsndfile-1"   PATHS ${DEPENDENCIES_DIR} NO_DEFAULT_PATH)
      find_library( dll_vorbis      NAMES "libvorbis"      PATHS ${DEPENDENCIES_DIR} NO_DEFAULT_PATH)
      find_library( dll_vorbisfile  NAMES "libvorbisfile"  PATHS ${DEPENDENCIES_DIR} NO_DEFAULT_PATH)
      find_library( dll_portaudio   NAMES "portaudio"      PATHS ${DEPENDENCIES_DIR} NO_DEFAULT_PATH)

      list(APPEND dlls_to_copy ${dll_lame} ${dll_ogg} ${dll_sndfile} ${dll_vorbis} ${dll_vorbisfile} ${dll_portaudio} ${dll_ssl1} ${dll_ssl2} "$<TARGET_FILE_DIR:muxaudio>/${MSCORE_OUTPUT_NAME}.exe")
      set( output_dir_for_dlls "${CMAKE_INSTALL_PREFIX}/bin")

      COPY_FILES_INTO_DIRECTORY_IF_CHANGED( "${dlls_to_copy}" ${output_dir_for_dlls}  muxaudio)

      # Keep dependencies in alphabetical order. Changes made to this list
      # might need to be made in "build/Linux+BSD/portable/copy-libs" too.
      get_filename_component(COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
      get_filename_component (MINGW_ROOT ${COMPILER_DIR} DIRECTORY)

      install( FILES
            ${dll_ogg}
            ${dll_lame}
            ${dll_sndfile}
            ${dll_portaudio}
            ${dll_vorbis}
            ${dll_vorbisfile}
            ${dll_ssl1}
            ${dll_ssl2}
            DESTINATION bin)

      install( TARGETS muxaudio RUNTIME DESTINATION bin ) # this duplicate due to the correctly package step

   endif ( NOT MSVC )
endif (MINGW)


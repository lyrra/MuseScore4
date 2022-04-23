# GPL-2.0-or-later

include_directories(
      ${PROJECT_SOURCE_DIR}/global
      ${PROJECT_SOURCE_DIR}/muxaudio/driver
      ${PROJECT_BINARY_DIR}
      ${QTSINGLEAPPLICATION_INCLUDE_DIRS}
      )

add_executable(muxaudio
      muxaudio.cpp
      muxaudio.h
      control.cpp
      control.h
      event.cpp
      driver/driver.cpp
      driver/driver.h
      driver/jackaudio.cpp
      driver/jackaudio.h
      driver/jackweakapi.cpp
      )

#if (MINGW OR MSVC)
#      set(MSCORE_OUTPUT_NAME ${MUSESCORE_NAME}${MUSESCORE_VERSION_MAJOR})
#elseif (MSCORE_INSTALL_SUFFIX)
#      set(MSCORE_OUTPUT_NAME "${ExecutableName}${MSCORE_INSTALL_SUFFIX}")
#endif (MINGW OR MSVC)

# If MSCORE_OUTPUT_NAME is set (e.g, when cmake is called by the user), the output executable will be
# called MSCORE_OUTPUT_NAME instead of 'mscore'. This can be used to have MuseScore stable and unstable
# both installed in the same prefix on a Linux system.
#if (MSCORE_OUTPUT_NAME)
#  set_target_properties(
#      ${ExecutableName}
#      PROPERTIES
#      OUTPUT_NAME ${MSCORE_OUTPUT_NAME}
#      )
#endif (MSCORE_OUTPUT_NAME)

target_include_directories(muxaudio PRIVATE
      ${PROJECT_SOURCE_DIR}
      )

target_link_libraries(muxaudio ${ZMQ_LIBRARY})


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

   target_link_libraries(muxaudio ${QT_LIBRARIES})

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
      ${PORTAUDIO_LIBRARY}
      DESTINATION bin)

else (MINGW)

   if ( NOT MSVC )
      set_target_properties (
         muxaudio
         PROPERTIES
            COMPILE_FLAGS "${PCH_INCLUDE} -g -Wall -Wno-overloaded-virtual -Winvalid-pch"
         )

      if (APPLE)
        target_link_libraries(muxaudio ${OsxFrameworks})
      else (APPLE)
          target_link_libraries(muxaudio rt)
      endif (APPLE)

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


#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2011-2016 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

# set(CMAKE_VERBOSE_MAKEFILE ON)
enable_testing()

if (MSVC)
      project(mtest)
      set (PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR}/..)
      set (PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/..)
endif (MSVC)

include_directories(
      ${QT_INCLUDES}
      ${PROJECT_BINARY_DIR}
      ${PROJECT_SOURCE_DIR}
      ${PROJECT_SOURCE_DIR}/thirdparty
      ${PROJECT_SOURCE_DIR}/muxlib
      ${PROJECT_SOURCE_DIR}/muxtools
      ${PROJECT_SOURCE_DIR}/muxseq
      ${PROJECT_SOURCE_DIR}/s7
      ${FREETYPE_INCLUDE_DIRS}
      ${GUILE_INCLUDE_DIRS}
      )
link_directories(${CMAKE_SOURCE_DIR}/lib)


if (OMR)
set(OMR_SRC ${PROJECT_SOURCE_DIR}/omr/importpdf.cpp)
endif (OMR)

if (MSVC)
   set(_all_h_file "${PROJECT_SOURCE_DIR}/all.h")
endif (MSVC)

QT5_ADD_RESOURCES(qrc_files ${PROJECT_SOURCE_DIR}/mtest/mtest.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Campania.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Leland.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Edwin.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-MScore.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Gootville.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Bravura.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-MuseJazz.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Free.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-FreeSerif.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-Petaluma.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-FinaleMaestro.qrc
            ${PROJECT_SOURCE_DIR}/mscore/musescorefonts-FinaleBroadway.qrc
      )

set (SOURCE_LIB
      ${qrc_files}
      testutils.cpp
      mtestglobals.cpp
      mscoreappresources.cpp

      ${PROJECT_SOURCE_DIR}/muxtools/exports/exportmidi.cpp
      ${PROJECT_SOURCE_DIR}/muxtools/exports/exportmidi.h
      ${PROJECT_SOURCE_DIR}/mscore/preferences.cpp
      ${PROJECT_SOURCE_DIR}/mscore/shortcut.cpp
      ${PROJECT_SOURCE_DIR}/thirdparty/rtf2html/fmt_opts.cpp        # Required by capella.cpp and capxml.cpp
      ${PROJECT_SOURCE_DIR}/thirdparty/rtf2html/rtf2html.cpp        # Required by capella.cpp and capxml.cpp
      ${PROJECT_SOURCE_DIR}/thirdparty/rtf2html/rtf_keyword.cpp     # Required by capella.cpp and capxml.cpp
      ${PROJECT_SOURCE_DIR}/thirdparty/rtf2html/rtf_table.cpp       # Required by capella.cpp and capxml.cpp
      ${PROJECT_SOURCE_DIR}/thirdparty/beatroot/Agent.cpp           # Required by importmidi.cpp
      ${PROJECT_SOURCE_DIR}/thirdparty/beatroot/AgentList.cpp       # Required by importmidi.cpp
      ${PROJECT_SOURCE_DIR}/thirdparty/beatroot/BeatTracker.cpp     # Required by importmidi.cpp
      ${PROJECT_SOURCE_DIR}/thirdparty/beatroot/Induction.cpp       # Required by importmidi.cpp
      ${PROJECT_SOURCE_DIR}/mscore/extension.cpp # required by zerberus tests
      ${OMR_SRC}
      omr
      )

if (MSVC)
    add_library(
      testutils SHARED
	  ${_all_h_file}
	  ${PCH}
	  ${SOURCE_LIB}
	  )
else (MSVC)
    add_library(
      testutils SHARED
	  ${SOURCE_LIB}
	  )
endif (MSVC)

target_link_libraries(testutils
    mscoreapp
    ${QT_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    )

set_target_properties (
    testutils
    PROPERTIES LINK_FLAGS "-g -Wl,--export-all-symbols"
    )

if (MSVC)
      target_sources(testutils_mscoreapp PRIVATE
            ${_all_h_file}
            ${PCH}
            )
endif (MSVC)

add_executable(
      mtest
      ${ui_headers}
      ${mocs}
      mtest.cpp
      s7mtest.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/s7gen.h ${CMAKE_CURRENT_BINARY_DIR}/s7gen.cpp
      )


target_link_libraries(
      mtest
      testutils
      s7
      mscoreapp
      ${FREETYPE_LIBRARIES}
      qzip
      ofqf
      ${QT_LIBRARIES}
      ${QT_QTTEST_LIBRARY}
      )

set_target_properties (
      mtest
      PROPERTIES
      AUTOMOC true
      COMPILE_FLAGS "-include all.h -D QT_GUI_LIB -D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\" -g -Wall -Wextra"
      LINK_FLAGS    "-g"
      )


if (GUILE)
      add_executable(
            ${_all_h_file}
            mtest-guile
            mtest-guile.cpp
            ${ui_headers}
            )
      target_link_libraries(
            mtest-guile
            testutils
            mscoreapp
            testutils
            ofqf
            qzip
            freetype
            z
            ${QT_QTTEST_LIBRARY}
            ${QT_LIBRARIES}
            )

      target_include_directories(mtest-guile PRIVATE
            ${QT_INCLUDES}
            ${GUILE_INCLUDE_DIRS})

      target_link_libraries(mtest-guile ${GUILE_LIBRARY})

endif (GUILE)


set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

string(REPLACE ";" ";-I" INC "${QT_INCLUDES}")

if (APPLE AND (CMAKE_VERSION VERSION_LESS "3.5.0"))
      set(TESTUTILS_COMPILE_FLAGS "-include all.h -D TESTROOT=\\\\\"${PROJECT_SOURCE_DIR}\\\\\" -g -Wall -Wextra")
else (APPLE AND (CMAKE_VERSION VERSION_LESS "3.5.0"))
      if (MSVC)
            set(TESTUTILS_COMPILE_FLAGS "${PCH_INCLUDE}  /D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\"")
      else (MSVC)
            set(TESTUTILS_COMPILE_FLAGS "-include all.h -D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\" -g -Wall -Wextra")
      endif (MSVC)
endif (APPLE AND (CMAKE_VERSION VERSION_LESS "3.5.0"))

set_target_properties(testutils PROPERTIES COMPILE_FLAGS "${TESTUTILS_COMPILE_FLAGS}")

if (NOT MSVC)
add_custom_target(report
      COMMAND ant -f ${PROJECT_SOURCE_DIR}/mtest/build.xml -Droot.dir=${PROJECT_BINARY_DIR}/mtest report
      WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/mtest"
      )

add_custom_target(reporthtml
      COMMAND ant -f ${PROJECT_SOURCE_DIR}/mtest/build.xml -Droot.dir=${PROJECT_BINARY_DIR}/mtest reporthtml
      WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/mtest"
      )

add_custom_target(mtest_build_and_install
      COMMAND ${CMAKE_COMMAND} --build . --target install
      WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/mtest"
      COMMENT "Build and install all mtests"
      )
endif (NOT MSVC)

add_custom_command (OUTPUT s7gen.h s7gen.cpp
                    COMMAND ${PROJECT_BINARY_DIR}/s7/s7 -f ${CMAKE_CURRENT_SOURCE_DIR}/testgen.scm -- --mtest-gen -l ${PROJECT_SOURCE_DIR}/s7 -s ${CMAKE_CURRENT_SOURCE_DIR} ../s7/lib.scm gen.scm types.scm ms.scm ms-objects.scm decl.scm -o ${CMAKE_CURRENT_BINARY_DIR}/s7gen
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
                    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/testgen.scm
                            ${CMAKE_CURRENT_SOURCE_DIR}/gen.scm
                            ${CMAKE_CURRENT_SOURCE_DIR}/types.scm
                            ${CMAKE_CURRENT_SOURCE_DIR}/ms.scm
                            ${CMAKE_CURRENT_SOURCE_DIR}/ms-objects.scm
                            ${CMAKE_CURRENT_SOURCE_DIR}/decl.scm
                            ${PROJECT_SOURCE_DIR}/s7/lib.scm
                    VERBATIM
                    )

add_custom_target(mtest_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/ms.scm ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/mtest.scm ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/types.scm ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/s7/lib.scm ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${PROJECT_BINARY_DIR}/s7/s7 -f ${CMAKE_CURRENT_SOURCE_DIR}/testgen.scm -- -l ${PROJECT_SOURCE_DIR}/s7 ${CMAKE_CURRENT_SOURCE_DIR}/libmscore/tst_element.scm -o ${CMAKE_CURRENT_BINARY_DIR}/libmscore/tst_element.scm

    COMMAND ${PROJECT_BINARY_DIR}/s7/s7 -f ${CMAKE_CURRENT_SOURCE_DIR}/testgen.scm -- -l ${PROJECT_SOURCE_DIR}/s7 ${CMAKE_CURRENT_SOURCE_DIR}/libmscore/tst_note.scm -o ${CMAKE_CURRENT_BINARY_DIR}/libmscore/tst_note.scm

    COMMAND ${PROJECT_BINARY_DIR}/s7/s7 -f ${CMAKE_CURRENT_SOURCE_DIR}/testgen.scm -- -l ${PROJECT_SOURCE_DIR}/s7 ${CMAKE_CURRENT_SOURCE_DIR}/libmscore/tst_hairpin.scm -o ${CMAKE_CURRENT_BINARY_DIR}/libmscore/tst_hairpin.scm

    COMMAND ${PROJECT_BINARY_DIR}/s7/s7 -f ${CMAKE_CURRENT_SOURCE_DIR}/testgen.scm -- -l ${PROJECT_SOURCE_DIR}/s7 ${CMAKE_CURRENT_SOURCE_DIR}/libmscore/tst_breath.scm -o ${CMAKE_CURRENT_BINARY_DIR}/libmscore/tst_breath.scm

    COMMAND ${PROJECT_BINARY_DIR}/s7/s7 -f ${CMAKE_CURRENT_SOURCE_DIR}/testgen.scm -- -l ${PROJECT_SOURCE_DIR}/s7 ${CMAKE_CURRENT_SOURCE_DIR}/libmscore/tst_copypaste.scm -o ${CMAKE_CURRENT_BINARY_DIR}/libmscore/tst_copypaste.scm

    DEPENDS libmscore/tst_element.scm
            libmscore/tst_note.scm
            libmscore/tst_hairpin.scm
            libmscore/tst_breath.scm
            libmscore/tst_copypaste.scm
)

subdirs (
        libmscore/compat114
        libmscore/compat206
#        libmscore/album            # obsolete
        libmscore/barline
        libmscore/beam
        libmscore/box
        libmscore/chordsymbol
        libmscore/clef
        libmscore/clef_courtesy
        libmscore/concertpitch
        libmscore/copypaste
        libmscore/copypastesymbollist
        libmscore/dynamic
        libmscore/durationtype
        libmscore/earlymusic
        libmscore/exchangevoices
        libmscore/implode_explode
        libmscore/instrumentchange
        libmscore/join
        libmscore/keysig
        libmscore/layout
        libmscore/layout_elements
        libmscore/links
        libmscore/parts
        libmscore/measure
        libmscore/midi                 # one disabled
#        libmscore/midimapping # TODO: compiles but mostly fails
        libmscore/note
        libmscore/readwriteundoreset
        libmscore/remove
        libmscore/repeat
        libmscore/rhythmicGrouping
        libmscore/selectionfilter
        libmscore/selectionrangedelete
        libmscore/unrollrepeats
        libmscore/spanners
        libmscore/split
        libmscore/splitstaff
        libmscore/tempotext
        libmscore/timesig
        libmscore/tools                # Some tests disabled
        libmscore/transpose
        libmscore/tuplet
#        libmscore/text        work in progress...
        libmscore/utils
        mscore/workspaces
        mscore/palette
        importmidi
        capella
        biab
        musicxml
        guitarpro
        scripting
        stringutils
#        testoves
        testscript
        )

configure_file(run-guile.sh
               ${PROJECT_BINARY_DIR}/mtest
               COPYONLY
               )

if (OMR)
subdirs(omr)
endif (OMR)

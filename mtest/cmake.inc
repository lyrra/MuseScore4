#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2011 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

add_executable(
      ${TARGET}
      ${ui_headers}
      ${mocs}
      ${TARGET}.cpp
      )

target_link_libraries(
      ${TARGET}
      testutils
      ${QT_LIBRARIES}
      ${QT_QTTEST_LIBRARY}
      qzip
      ofqf
      )

if (OMR)
      target_link_libraries(${TARGET} omr poppler-qt5)
      if (OCR)
            target_link_libraries(${TARGET} tesseract_api)
      endif (OCR)
endif (OMR)

if (APPLE)
target_link_libraries(${TARGET}
      ${OsxFrameworks}
      dl
      pthread
      )
set_target_properties (
      ${TARGET}
      PROPERTIES
      AUTOMOC true
      COMPILE_FLAGS "-include all.h -D QT_GUI_LIB -D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\" -g -Wall -Wextra"
      LINK_FLAGS    "-g -stdlib=libc++"
      )
else(APPLE)
      if (MSVC)
            set_target_properties (
                  ${TARGET}
                  PROPERTIES
                  AUTOMOC true
                  COMPILE_FLAGS "/Zi ${PCH_INCLUDE} /D QT_GUI_LIB  /D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\""
                  )
      else (MSVC)
            set_target_properties (
                  ${TARGET}
                  PROPERTIES
                  AUTOMOC true
                  COMPILE_FLAGS "-include all.h -D QT_GUI_LIB -D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\" -g -Wall -Wextra"
                  LINK_FLAGS    "-g"
                  )
      endif (MSVC)
endif(APPLE)

if (APPLE AND (CMAKE_VERSION VERSION_LESS "3.5.0"))
set_target_properties (
      ${TARGET}
      PROPERTIES
      COMPILE_FLAGS "-include all.h -D QT_GUI_LIB -D TESTROOT=\\\\\"${PROJECT_SOURCE_DIR}\\\\\" -g -Wall -Wextra"
      )
endif (APPLE AND (CMAKE_VERSION VERSION_LESS "3.5.0"))

add_test(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}  -xunitxml -o result.xml)

if (GUILE)
      target_link_libraries(${TARGET} ${GUILE_LIBRARIES})
      target_link_directories(${TARGET} PRIVATE ${GUILE_LIBRARY_DIRS})
endif (GUILE)

# On Windows some tests need access to supporting files
# MSVC has a different definition for CMAKE_CURRENT_BINARY_DIR
# so we use this install destination variable instead
if (MSVC)
      set_target_properties(
            ${TARGET}
            PROPERTIES
            MTEST_INSTALL_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}"
            )
else (MSVC)
      set_target_properties(
            ${TARGET}
            PROPERTIES
            MTEST_INSTALL_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/.."
            )
endif (MSVC)

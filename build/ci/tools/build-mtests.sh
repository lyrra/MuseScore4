#!/usr/bin/env bash

set -e

echo "Build MuseScore mtest"

source ./../musescore_environment.sh

mkdir build.debug 2> /dev/null
cd build.debug

cmake -G "Unix Makefiles" \
      -DCMAKE_INSTALL_PREFIX=install \
      -DCMAKE_BUILD_TYPE=DEBUG \
      -DVERBOSE=1 \
      -DBUILD_SHARED=ON \
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
      -DDOWNLOAD_SOUNDFONT=ON \
      -DBUILD_WEBENGINE=OFF \
      -DLOGLEVEL=4 \
      ..

cd mtest

# run the mtests in "minimal" platform for headless systems
# enable fonts handling
export QT_QPA_PLATFORM=minimal:enable_fonts
# if AddressSanitizer was used, disable leak detection
export ASAN_OPTIONS=detect_leaks=0:new_delete_type_mismatch=0

export GUILE_SYSTEM_COMPILED_PATH=/mingw64/lib/guile/3.0/ccache

make


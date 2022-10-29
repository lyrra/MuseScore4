#!/usr/bin/env bash

#export GUILE_SYSTEM_COMPILED_PATH=/usr/lib/x86_64-linux-gnu/guile/3.0/ccache
#export CFLAGS="-fsanitize=address -fno-omit-frame-pointer"
#export CXXFLAGS="-fsanitize=address -fno-omit-frame-pointer"

echo "Build Linux MuseScore"

mkdir build.debug 2> /dev/null
cd build.debug || exit 1

cmake -G "Unix Makefiles" \
      -DCMAKE_INSTALL_PREFIX=install \
      -DCMAKE_BUILD_TYPE=DEBUG \
      -DVERBOSE=1 \
      -DBUILD_SHARED=ON \
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
      -DBUILD_FOR_WINSTORE=OFF \
      -DBUILD_CRASH_REPORTER=OFF \
      -DBUILD_PULSEAUDIO=OFF \
      -DDOWNLOAD_SOUNDFONT=ON \
      -DBUILD_WEBENGINE=OFF \
      -DLOGLEVEL=4 \
      ..

make

make install

#make CPUS=2 $OPTIONS \
#     MUSESCORE_BUILD_CONFIG=$MUSESCORE_BUILD_CONFIG \
#     MUSESCORE_REVISION=$MUSESCORE_REVISION \
#     BUILD_NUMBER=$BUILD_NUMBER \
#     TELEMETRY_TRACK_ID=$TELEMETRY_TRACK_ID \
#     SUFFIX=$SUFFIX \
#     $BUILDTYPE

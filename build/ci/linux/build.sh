#!/usr/bin/env bash

echo "Build Linux MuseScore AppImage"

mkdir build.debug 2> /dev/null
cd build.debug || exit 1

cmake -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=DEBUG \
      -DVERBOSE=1 \
      -DBUILD_SHARED=ON \
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
      -DBUILD_FOR_WINSTORE=OFF \
      -DLOGLEVEL=1 \
      ..

make


#make CPUS=2 $OPTIONS \
#     MUSESCORE_BUILD_CONFIG=$MUSESCORE_BUILD_CONFIG \
#     MUSESCORE_REVISION=$MUSESCORE_REVISION \
#     BUILD_NUMBER=$BUILD_NUMBER \
#     TELEMETRY_TRACK_ID=$TELEMETRY_TRACK_ID \
#     SUFFIX=$SUFFIX \
#     $BUILDTYPE

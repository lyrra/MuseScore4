#!/usr/bin/env bash

echo Current dir:
pwd

C=`pwd`

cd build.debug

echo "------ Files to zip: ---------"

echo "------ zipping files: ---------"

find . | \
     grep -e 's7/s7.exe' \
          -e 's7/libs7.a' \
          -e 'muxaudio/muxaudio.exe' \
          -e 'muxseq/muxseq.exe' \
          -e 'muxtools/libmuxtools.a' \
          -e 'muxlib/libmuxlib.a' \
          -e 'libmscore/liblibmscore.a' \
          -e 'mscore/libmscoreapp.a' \
          -e 'main/MuseScore3.exe' \
          -e 'main/libMuseScore3.dll.a' \
          -e 'mtest' \
          -e '../script' \
          -e '../fonts' \
          -e 'share' \
          -e '^./README.md' \
          -e '^./LICENSE.GPL' \
     | zip ../musescore3-mingw64.zip -@

ls -ltr ../musescore3-mingw64.zip

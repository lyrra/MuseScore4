#!/usr/bin/env bash

set -e

echo "Build MuseScore mtest"

source ./../musescore_environment.sh

#export CFLAGS="-fsanitize=address -fno-omit-frame-pointer"
#export CXXFLAGS="-fsanitize=address -fno-omit-frame-pointer"

cd build.debug/mtest

make


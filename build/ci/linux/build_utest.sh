#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-CLA-applies
#
# MuseScore
# Music Composition & Notation
#
# Copyright (C) 2021 MuseScore BVBA and others
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
echo "Build utests"

#set -x
trap 'echo Build failed; exit 1' ERR

df -h .

ARTIFACTS_DIR=build.artifacts
BUILD_NUMBER=42

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -n|--number) BUILD_NUMBER="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

cat ./../musescore_environment.sh
source ./../musescore_environment.sh

echo " "
${CXX} --version 
${CC} --version
echo " "
cmake --version
echo " "
echo "=== BUILD ==="

MUSESCORE_REVISION=$(git rev-parse --short=7 HEAD)

MUSESCORE_BUILD_CONFIG=dev \
MUSESCORE_BUILD_NUMBER=$BUILD_NUMBER \
MUSESCORE_REVISION=$MUSESCORE_REVISION \
MUSESCORE_BUILD_VST=$BUILD_VST \
MUSESCORE_VST3_SDK_PATH=$VST3_SDK_PATH \
MUSESCORE_DOWNLOAD_SOUNDFONT=OFF \
MUSESCORE_BUILD_UNIT_TESTS=ON \

#-----------------------------------------------------------------------
VERSION=$(cmake -P config.cmake | sed -n -e "s/^.*VERSION  *//p")
CPUS=4
export VERBOSE=1

mkdir build.debug 2> /dev/null
cd build.debug || exit 1

cmake -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=DEBUG \
      -DVERBOSE=1 \
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
      -DBUILD_FOR_WINSTORE=OFF \
      -DBUILD_TELEMETRY_MODULE=OFF \
      -DBUILD_CRASH_REPORTER=OFF \
      -DLOGLEVEL=9 \
      ..
make

df -h .

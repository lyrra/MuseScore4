#!/usr/bin/env bash

set -e

echo "Build MuseScore mtest"

source ./../musescore_environment.sh

echo "----------- environment variables --------------"
env
echo "------------------------------------------------"

cd build.debug/mtest

# run the mtests in "minimal" platform for headless systems
# enable fonts handling
export QT_QPA_PLATFORM=minimal:enable_fonts
# if AddressSanitizer was used, disable leak detection
#export ASAN_OPTIONS=detect_leaks=0:new_delete_type_mismatch=0


make


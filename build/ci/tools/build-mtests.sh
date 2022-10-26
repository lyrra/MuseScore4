#!/usr/bin/env bash

echo "Build MuseScore mtest"
trap 'echo Run tests failed; exit 1' ERR

source ./../musescore_environment.sh

cd build.debug/mtest

# run the mtests in "minimal" platform for headless systems
# enable fonts handling
export QT_QPA_PLATFORM=minimal:enable_fonts
# if AddressSanitizer was used, disable leak detection
export ASAN_OPTIONS=detect_leaks=0:new_delete_type_mismatch=0

export GUILE_SYSTEM_COMPILED_PATH=/mingw64/lib/guile/3.0/ccache

make


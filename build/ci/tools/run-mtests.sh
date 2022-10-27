#!/usr/bin/env bash

set -e

echo "Run MuseScore mtest"

source ./../musescore_environment.sh

D=`pwd`/build.debug

cd build.debug/mtest

# run the mtests in "minimal" platform for headless systems
# enable fonts handling
export QT_QPA_PLATFORM=minimal:enable_fonts
# if AddressSanitizer was used, disable leak detection
export ASAN_OPTIONS=detect_leaks=0:new_delete_type_mismatch=0
export LD_LIBRARY_PATH=$D/muxtools:$D/libmscore:$D/muxlib:$D/mscore:$D/importexport:$D/mtest

#echo "---- run ctest ----"
#ctest --output-on-failure
echo "---- run mtest ----"
ldd mtest
./mtest $*
echo "---- run guile tests ----"
sh run-guile.sh $*


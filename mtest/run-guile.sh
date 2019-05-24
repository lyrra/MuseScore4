#!/bin/sh

# Run this script from the build.debug/mtest directory

GUILE_LOAD_PATH="../../script/guile"
export GUILE_LOAD_PATH

./mtest-guile $*


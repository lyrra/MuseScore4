#!/bin/sh

VERSION=$(cmake -P config.cmake | sed -n -e "s/^.*VERSION  *//p")
BUILD_NUMBER=""
#CPUS=1

export VERBOSE=1

mkdir build.debug 2> /dev/null
cd build.debug || exit 1

echo "*** PATH: $PATH"
echo "*** Running cmake ***"
command -v cmake
cmake -G "MinGW Makefiles" \
      -DCMAKE_INSTALL_PREFIX=../win32install \
      -DCMAKE_BUILD_TYPE=DEBUG \
      -DVERBOSE=1 \
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
      -DCMAKE_MAKE_PROGRAM=mingw32-make.exe \
      -DBUILD_FOR_WINSTORE=OFF \
      .. || exit 1

echo "*** Running make ***"
command -v make
make || exit 1

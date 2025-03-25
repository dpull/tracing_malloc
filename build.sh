#!/bin/sh
#set -x
set -e

BUILD_TYPE="RelWithDebInfo"

if [ $# -gt 0 ]; then
    if [ "$1" = "Debug" ] || [ "$1" = "RelWithDebInfo" ]; then
        BUILD_TYPE="$1"
    else
        echo "Usage: $0 [Debug|RelWithDebInfo]"
        exit 1
    fi
fi

echo "Building in $BUILD_TYPE mode..."

if [ ! -d build ]; then
    mkdir build
fi

cd build

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../src

make -j64

cd -

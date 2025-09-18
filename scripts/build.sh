#!/usr/bin/env bash
set -e

# Remove old build dir
rm -rf build
mkdir build
cd build

# Detect platform
UNAME=$(uname | tr '[:upper:]' '[:lower:]')

# Set CMake generator and build command
CMAKE_GENERATOR=""
BUILD_CMD=""

if [[ "$UNAME" == "linux" ]]; then
    CMAKE_GENERATOR="Unix Makefiles"
    BUILD_CMD="make -j$(nproc)"
elif [[ "$UNAME" == darwin* ]]; then
    CMAKE_GENERATOR="Unix Makefiles"
    BUILD_CMD="make -j$(sysctl -n hw.ncpu)"
elif [[ "$UNAME" == msys* || "$UNAME" == mingw* || "$UNAME" == cygwin* ]]; then
    # Windows (MSYS2 / MinGW)
    if command -v cmake &>/dev/null; then
        CMAKE_GENERATOR="MinGW Makefiles"
        BUILD_CMD="mingw32-make -j$(nproc)"
    else
        echo "CMake not found in PATH!"
        exit 1
    fi
else
    echo "Unsupported platform: $UNAME"
    exit 1
fi

echo "Using generator: $CMAKE_GENERATOR"
cmake -G "$CMAKE_GENERATOR" ..

echo "Building..."
$BUILD_CMD

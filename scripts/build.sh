#!/usr/bin/env bash
set -e

source .env

DEBUG=false
BUILD_TYPE=Release
CLEAN_BUILD=false
BUILD_DIR=build

for arg in "$@"; do
    case $arg in
        --debug)
            BUILD_TYPE=Debug
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --workflow)
            source .env_gh
            CLEAN_BUILD=true
            shift
            ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: ./build.sh [--clean] [--debug]"
            exit 1
            ;;
    esac
done

if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build folder..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

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
cmake -G "$CMAKE_GENERATOR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..

echo "Building..."
$BUILD_CMD

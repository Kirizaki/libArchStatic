cmake_minimum_required(VERSION 3.16)
project(libArchStatic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(libArchStatic
  main.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

if(WIN32)
    find_library(LIBARCHIVE_LIB NAMES archive PATHS /mingw64/lib)
    find_library(ZSTD_LIB NAMES zstd PATHS /mingw64/lib)
    find_library(LZMA_LIB NAMES lzma PATHS /mingw64/lib)
    find_library(BZIP2_LIB NAMES bz2 PATHS /mingw64/lib)
    find_library(ZLIB_LIB NAMES z PATHS /mingw64/lib)
    find_library(OPENSSL_LIB NAMES crypto PATHS /mingw64/lib)

    target_include_directories(libArchStatic PRIVATE /mingw64/include)

    target_link_libraries(libArchStatic PRIVATE
        ${LIBARCHIVE_LIB}
        ${ZSTD_LIB}
        ${LZMA_LIB}
        ${BZIP2_LIB}
        ${ZLIB_LIB}
        ${OPENSSL_LIB}
    )
elseif(APPLE)
    # headers/includes:
    set(LIBARCHIVE_H $ENV{LIBARCHIVE_H} CACHE PATH "libarchive headers path")
    set(ZSTD_H       $ENV{ZSTD_H}       CACHE PATH "zstd headers path")
    set(XZ_H         $ENV{XZ_H}         CACHE PATH "xz headers path")
    set(BZIP2_H      $ENV{BZIP2_H}      CACHE PATH "bzip2 headers path")
    set(ZLIB_H       $ENV{ZLIB_H}       CACHE PATH "zlib headers path")
    set(OPENSSL_H    $ENV{OPENSSL_H}    CACHE PATH "openssl headers path")
    # libs:
    set(LIBARCHIVE_LIB $ENV{LIBARCHIVE_LIB} CACHE PATH "libarchive lib path")
    set(ZSTD_LIB       $ENV{ZSTD_LIB}       CACHE PATH "zstd lib path")
    set(XZ_LIB         $ENV{XZ_LIB}         CACHE PATH "xz lib path")
    set(BZIP2_LIB      $ENV{BZIP2_LIB}      CACHE PATH "bzip2 lib path")
    set(ZLIB_LIB       $ENV{ZLIB_LIB}       CACHE PATH "zlib lib path")
    set(OPENSSL_LIB    $ENV{OPENSSL_LIB}    CACHE PATH "openssl lib path")

    target_include_directories(libArchStatic PRIVATE
        ${LIBARCHIVE_H}
        ${ZSTD_H}
        ${XZ_H}
        ${BZIP2_H}
        ${ZLIB_H}
        ${OPENSSL_H}
    )

    target_link_libraries(libArchStatic PRIVATE
        ${LIBARCHIVE_LIB}
        ${ZSTD_LIB}
        ${XZ_LIB}
        ${BZIP2_LIB}
        ${ZLIB_LIB}
        ${OPENSSL_LIB}
    )

elseif(UNIX)
    message(FATAL_ERROR "Unsupported platform")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

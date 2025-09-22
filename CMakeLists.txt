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
    set(LIBARCHIVE_ROOT $ENV{LIBARCHIVE_ROOT} CACHE PATH "libarchive root path")
    set(ZSTD_ROOT       $ENV{ZSTD_ROOT}       CACHE PATH "zstd root path")
    set(XZ_ROOT         $ENV{XZ_ROOT}         CACHE PATH "xz root path")
    set(BZIP2_ROOT      $ENV{BZIP2_ROOT}      CACHE PATH "bzip2 root path")
    set(ZLIB_ROOT       $ENV{ZLIB_ROOT}       CACHE PATH "zlib root path")
    set(OPENSSL_ROOT    $ENV{OPENSSL_ROOT}    CACHE PATH "openssl root path")
    target_include_directories(libArchStatic PRIVATE
        ${LIBARCHIVE_ROOT}/include
        ${ZSTD_ROOT}/include
        ${XZ_ROOT}/include
        ${BZIP2_ROOT}/include
        ${ZLIB_ROOT}/include
        ${OPENSSL_ROOT}/include
    )

    target_link_libraries(libArchStatic PRIVATE
        ${LIBARCHIVE_ROOT}/lib/libarchive.a
        ${ZSTD_ROOT}/lib/libzstd.a
        ${XZ_ROOT}/lib/liblzma.a
        ${BZIP2_ROOT}/lib/libbz2.a
        ${ZLIB_ROOT}/lib/libz.a
        ${OPENSSL_ROOT}/lib/libcrypto.a
        ${OPENSSL_ROOT}/lib/libssl.a
    )

elseif(UNIX)
    message(FATAL_ERROR "Unsupported platform")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

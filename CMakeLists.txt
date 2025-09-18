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
    set(MSYS2_PREFIX "/c/msys64/mingw64")

    target_include_directories(libArchStatic PRIVATE
        ${MSYS2_PREFIX}/include
        ${MSYS2_PREFIX}/include/libarchive
    )

    target_link_libraries(libArchStatic PRIVATE
        ${MSYS2_PREFIX}/lib/libarchive.a
        ${MSYS2_PREFIX}/lib/libzstd.a
        ${MSYS2_PREFIX}/lib/liblzma.a
        ${MSYS2_PREFIX}/lib/libbz2.a
        ${MSYS2_PREFIX}/lib/libz.a
        ${MSYS2_PREFIX}/lib/libcrypto.a
    )
elseif(APPLE)
    set(LIBARCHIVE_ROOT $ENV{LIBARCHIVE_ROOT} CACHE PATH "libarchive root path")
    set(ZSTD_ROOT       $ENV{ZSTD_ROOT}       CACHE PATH "zstd root path")
    set(XZ_ROOT         $ENV{XZ_ROOT}         CACHE PATH "xz root path")
    set(BZIP2_ROOT      $ENV{BZIP2_ROOT}      CACHE PATH "bzip2 root path")
    set(ZLIB_ROOT       $ENV{ZLIB_ROOT}       CACHE PATH "zlib root path")
    set(OPENSSL_ROOT    $ENV{OPENSSL_ROOT}    CACHE PATH "openssl root path")

    target_include_directories(libArchStatic PRIVATE
        ${LIBARCHIVE_ROOT}/libarchive
        ${ZSTD_ROOT}/include
        ${XZ_ROOT}/include
        ${BZIP2_ROOT}/include
        ${ZLIB_ROOT}/include
        ${OPENSSL_ROOT}/include
    )

    target_link_libraries(libArchStatic PRIVATE
        ${LIBARCHIVE_ROOT}/build/libarchive/libarchive.a
        ${ZSTD_ROOT}/lib/libzstd.a
        ${XZ_ROOT}/lib/liblzma.a
        ${BZIP2_ROOT}/lib/libbz2.a
        ${ZLIB_ROOT}/lib/libz.a
        ${OPENSSL_ROOT}/lib/libcrypto.a
    )

elseif(UNIX)
    message(FATAL_ERROR "Unsupported platform")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

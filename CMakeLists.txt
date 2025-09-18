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
    # Use vcpkg toolchain: vcpkg automatically sets include/lib paths
    find_package(libarchive REQUIRED CONFIG)
    find_package(zstd REQUIRED CONFIG)
    find_package(bzip2 REQUIRED CONFIG)
    find_package(xz REQUIRED CONFIG)
    find_package(ZLIB REQUIRED CONFIG)
    find_package(OpenSSL REQUIRED CONFIG)

    target_link_libraries(libArchStatic PRIVATE
        libarchive::libarchive
        ZSTD::ZSTD
        BZip2::BZip2
        XZ::XZ
        ZLIB::ZLIB
        OpenSSL::SSL
        OpenSSL::Crypto
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

cmake_minimum_required(VERSION 3.16)

project(libArchStatic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBARCHIVE_ROOT /Users/greg/repo/libarchive)
set(ZSTD_ROOT /opt/homebrew/opt/zstd)
set(XZ_ROOT /opt/homebrew/opt/xz)
set(BZIP2_ROOT /opt/homebrew/opt/bzip2)
set(ZLIB_ROOT /opt/homebrew/opt/zlib)
set(OPENSSL_ROOT /opt/homebrew/opt/openssl@3)

add_executable(libArchStatic
  main.cpp
)
target_include_directories(libArchStatic PRIVATE
    ${LIBARCHIVE_ROOT}/libarchive
    ${ZSTD_ROOT}/include
    ${XZ_ROOT}/include
    ${BZIP2_ROOT}/include
    ${ZLIB_ROOT}/include
    ${OPENSSL_ROOT}/include
)
target_link_libraries(libArchStatic
    ${LIBARCHIVE_ROOT}/build/libarchive/libarchive.a
    ${ZSTD_ROOT}/lib/libzstd.a
    ${XZ_ROOT}/lib/liblzma.a
    ${BZIP2_ROOT}/lib/libbz2.a
    ${ZLIB_ROOT}/lib/libz.a
    ${OPENSSL_ROOT}/lib/libcrypto.a
)

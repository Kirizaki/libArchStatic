cmake_minimum_required(VERSION 3.16)
project(libArchStatic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(libArchStatic
  main.cpp
)

target_compile_definitions(libArchStatic PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

# Platform-specific configuration
if(WIN32)
    # Find prebuilt static libraries
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
    # MacOS Homebrew paths
    set(LIBARCHIVE_ROOT "/Users/greg/repo/libarchive")
    set(ZSTD_ROOT "/opt/homebrew/opt/zstd")
    set(XZ_ROOT "/opt/homebrew/opt/xz")
    set(BZIP2_ROOT "/opt/homebrew/opt/bzip2")
    set(ZLIB_ROOT "/opt/homebrew/opt/zlib")
    set(OPENSSL_ROOT "/opt/homebrew/opt/openssl@3")

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
    # Linux paths (example: installed under /usr/local)
    set(LIBARCHIVE_ROOT "/usr/local")
    set(ZSTD_ROOT "/usr/local")
    set(XZ_ROOT "/usr/local")
    set(BZIP2_ROOT "/usr/local")
    set(ZLIB_ROOT "/usr/local")
    set(OPENSSL_ROOT "/usr/local/openssl")

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
    )

else()
    message(FATAL_ERROR "Unsupported platform")
endif()

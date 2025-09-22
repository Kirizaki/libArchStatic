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
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
    pkg_check_modules(ZSTD REQUIRED libzstd)
    pkg_check_modules(XZ REQUIRED liblzma)
    pkg_check_modules(BZIP2 REQUIRED bzip2)
    pkg_check_modules(ZLIB REQUIRED zlib)
    pkg_check_modules(OpenSSL REQUIRED openssl)

    target_link_libraries(libArchStatic PRIVATE
        ${LIBARCHIVE_LIBRARIES}
        ${ZSTD_LIBRARIES}
        ${XZ_LIBRARIES}
        ${BZIP2_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${OPENSSL_LIBRARIES}
    )
    target_include_directories(libArchStatic PRIVATE
        ${LIBARCHIVE_INCLUDE_DIRS}
        ${ZSTD_INCLUDE_DIRS}
        ${XZ_INCLUDE_DIRS}
        ${BZIP2_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIRS}
    )

elseif(UNIX)
    message(FATAL_ERROR "Unsupported platform")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

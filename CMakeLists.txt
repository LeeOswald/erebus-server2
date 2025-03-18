cmake_minimum_required(VERSION 3.20)


if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build configuration specified, defaulting to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(version)
get_version_from_file("${CMAKE_CURRENT_SOURCE_DIR}/version.txt" ER_VERSION_)
set(ER_VERSION ${ER_VERSION_} CACHE STRING "Erebus server version determined from version file" FORCE)
message(STATUS "Determined Erebus server version ${ER_VERSION}")

project(erebus-server VERSION ${ER_VERSION})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DER_64=1)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_definitions(-DER_32=1)
else()
    message(FATAL_ERROR "Unsupported bitness")
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/a")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(debug)

# detect OS
if(WIN32)
    set(ER_WINDOWS TRUE)
    add_definitions(-DER_WINDOWS=1)
elseif(UNIX AND NOT APPLE)
    set(ER_LINUX TRUE)
    set(ER_POSIX TRUE)
    add_definitions(-DER_LINUX=1)
    add_definitions(-DER_POSIX=1)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

# -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Boost
find_package(Boost REQUIRED)
add_compile_definitions(BOOST_STACKTRACE_LINK)
if(ER_POSIX)
    add_compile_definitions(BOOST_STACKTRACE_USE_BACKTRACE)
endif()
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# {fmt}
find_package(fmt REQUIRED)

# {fmt} wants this
if(MSVC)
    add_compile_options("/utf-8")
endif()

# GTest
find_package(GTest REQUIRED)

# lua
find_package(Lua REQUIRED)
include_directories(${lua_INCLUDE_DIRS})


# rapidjson
find_package(RapidJSON REQUIRED)

# valijson
find_package(valijson REQUIRED)

# protobuf
find_package(protobuf REQUIRED)

# gRPC
find_package(gRPC CONFIG REQUIRED)


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/erebus-version.h.in" erebus-version.h)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
    "${PROJECT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Windows-specific settings
if(ER_WINDOWS)
    add_definitions(-D_WIN32_WINNT=0x0602)
endif()

add_subdirectory(src)

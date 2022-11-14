cmake_minimum_required(VERSION 3.2)

project(hgs
        VERSION 1.0.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES "x86_64")

# Ignore unused parameters, because the LS operators depend on this (sometimes
# the parameters are used, but not by every operator)
add_definitions(-Werror -Wall -Wextra -Wpedantic -fPIC -Wno-unused-parameter)

if (CMAKE_BUILD_TYPE MATCHES RELEASE)
    add_definitions(-O3 -flto)
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)
include_directories(include/operators)
include_directories(include/stop)

add_subdirectory(extern)
add_subdirectory(src)

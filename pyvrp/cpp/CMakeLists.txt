cmake_minimum_required(VERSION 3.15)

project(pyvrp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif ()

find_package(pybind11 REQUIRED)

# Ignore unused parameters, because the LS operators depend on this (sometimes
# the parameters are used, but not by every operator)
add_definitions(-Werror -Wall -Wextra -Wpedantic -fPIC -Wno-unused-parameter)

if (CMAKE_BUILD_TYPE MATCHES RELEASE)
    add_definitions(-O3 -flto)
else ()
    # For code coverage on the test builds
    add_definitions(-fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif ()

include_directories(include)
add_subdirectory(src)

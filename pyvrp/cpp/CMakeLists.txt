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

# Define CMAKE_INSTALL_xxx: LIBDIR, INCLUDEDIR
include(GNUInstallDirs)
include(FetchContent)

# Include pybind11
FetchContent_Declare(
        pybind11
        URL https://github.com/pybind/pybind11/archive/refs/tags/v2.9.2.tar.gz
        URL_HASH SHA256=6bd528c4dbe2276635dc787b6b1f2e5316cf6b49ee3e150264e455a0d68d19c1
)
FetchContent_MakeAvailable(pybind11)

# Ignore unused parameters, because the LS operators depend on this (sometimes
# the parameters are used, but not by every operator)
add_definitions(-Werror -Wall -Wextra -Wpedantic -fPIC -Wno-unused-parameter)

if (CMAKE_BUILD_TYPE MATCHES RELEASE)
    add_definitions(-O3 -flto)
else ()
    # TODO switch to kcov for code coverage? https://github.com/SimonKagstrom/kcov
    # For code coverage on the test builds
    add_definitions(-fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif ()

include_directories(include)
add_subdirectory(src)

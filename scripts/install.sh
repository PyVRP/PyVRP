#!/usr/bin/env bash

# This script builds and installs the pyvrp project. Any arguments are passed
# to the compilation step.

set -e;  # exit immediately if any command fails

BUILD_DIR="build";

# Command line argument for buildtype. Supports a few options, including 
# 'debug' and 'release'. If not provided, a debug build is generated.
BUILD_TYPE=${1:-debug};
shift;

params=(
    --buildtype "$BUILD_TYPE"
    -Dpython.platlibdir="$PWD"
    "$@"
)

if [ ! -d "build" ];  # does a build directory already exist?
then
    meson setup "$BUILD_DIR" "${params[@]}";
else
    meson setup "$BUILD_DIR" --reconfigure "${params[@]}";
fi

meson compile -C "$BUILD_DIR";
meson install -C "$BUILD_DIR";

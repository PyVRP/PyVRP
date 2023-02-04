#!/usr/bin/env bash

# This script builds and installs the pyvrp project in the local poetry 
# environment.

set -e;  # exit immediately if any command fails

# if --clean is passed, remove the current build directory and previously 
# installed binaries, and perform a clean install.
if [[ $* == *--clean* ]]
then
    rm -rf build;
    rm pyvrp/**/*.so;
fi

poetry install;

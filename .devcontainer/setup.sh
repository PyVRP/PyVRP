#!/bin/bash

# Install g++-11
sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install g++-11
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100

# Install poetry
pipx install poetry

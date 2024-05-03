#!/bin/bash

# Add ~/.local/bin to PATH for pip-installed executables
echo 'export PATH=~/.local/bin:$PATH' >> ~/.bashrc

# Install g++-11
sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install g++-11
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100

# Install recent CMake version
pipx install cmake

# Install poetry
pipx install poetry

#!/bin/bash

# Add ~/.local/bin to PATH for pip-installed executables
echo 'export PATH=~/.local/bin:$PATH' >> ~/.bashrc

# Install g++-13
sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install g++-13
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Install recent CMake version
pipx install cmake

# Install poetry
pipx install poetry

# Install package dependencies
poetry install

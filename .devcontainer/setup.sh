#!/bin/bash

# Install Python and pip
sudo apt-get install software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt-get update
sudo apt-get install python3.10 python3-pip

# Install poetry
curl -sSL https://install.python-poetry.org | python3 -
poetry config virtualenvs.in-project true

# Initialize submodules
git submodule update --init

# Don't ask for superuser acces when debugging extensions
# See https://github.com/benibenj/vscode-pythonCpp/issues/18
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope

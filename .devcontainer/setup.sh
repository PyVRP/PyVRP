#!/bin/bash

# Install Python3.10
sudo apt-get install software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt-get update
sudo apt-get install python3.10

# Install poetry
python3 -m pip install --user pipx
python3 -m pipx ensurepath
sudo pipx ensurepath --global
pipx install poetry

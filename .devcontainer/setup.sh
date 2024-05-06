#!/bin/bash

# Install Python and pip
sudo apt-get install software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt-get update
sudo apt-get install python3.10 python3-pip

# Install pipx
python3 -m pip install --user pipx
python3 -m pipx ensurepath
sudo pipx ensurepath --global

# Install poetry
pipx install poetry

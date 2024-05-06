#!/bin/bash

# Install Python and pip
sudo apt-get install software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt-get update
sudo apt-get install python3.10 python3-pip

# Install poetry
curl -sSL https://install.python-poetry.org | python3 -

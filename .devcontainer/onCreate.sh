#!/bin/bash

set -e

git submodule update --init

uv sync --no-install-project
uv sync --group docs --group examples 
rm -rf build/
uv run pre-commit install

#!/bin/bash

set -e

git submodule update --init

uv sync --group docs --group examples
uv run pre-commit install

#!/bin/bash

set -e

git submodule update --init

poetry install --with examples,docs,dev
poetry run pre-commit install

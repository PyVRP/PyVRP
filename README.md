[![CI](https://github.com/N-Wouda/pyvrp/actions/workflows/CI.yml/badge.svg?branch=main)](https://github.com/N-Wouda/pyvrp/actions/workflows/CI.yml)
[![codecov](https://codecov.io/gh/N-Wouda/pyvrp/branch/main/graph/badge.svg?token=G9JKIVZOHB)](https://codecov.io/gh/N-Wouda/pyvrp)

⚠️⚠️⚠️ **This package is under heavy development - expect things to break!** ⚠️⚠️⚠️ 

# PyVRP

The `pyvrp` package is an open-source, state-of-the-art vehicle routing problem solver.

## Local installation

You will need a recent/modern C++ compiler. 
Any recent version of gcc, clang, or msvc should do.
We also use poetry extensively. 
If you do not have poetry, you can get it via
```shell
pip install poetry
```
Now we need to install all dependencies into the local environment:
```shell
poetry install
```
This command does two things: first, it installs all dependencies that are required for developing `pyvrp`.
Second, it installs the `pyvrp` package in editable mode in the poetry environment.
Setting up the poetry environment and installing the `pyvrp` package takes a little while, but most of it only needs to be done once.
When the command finishes, you can verify everything went correctly by running
```shell
poetry run pytest
```
If all tests pass without errors, you have a working installation of the codebase.

From this point onwards, recompilation of the C++ extensions can best be done using the `scripts/install.sh` script.
It can be called as
```shell
poetry run scripts/install.sh <buildtype>
``` 
It also takes an optional build type argument: valid options include `debug` (default), and `release`.

### Details

We use the Meson build system to compile the C++ extensions.
Meson is configured using the `meson.build` file in the repository root. 
You should not have to touch this file often: all installation is handled via the `scripts/install.sh` script.
For deployment, we use the [`pypa/build`](https://github.com/pypa/build) frontend.
The first uses (via `poetry-core`, as defined under `[build-system]` in the `pyproject.toml` file) the `build_extension.py` file to call into `scripts/install.sh`.

In short: (for now) everything runs via `scripts/install.sh`.

### Build system

Any recent compiler should do.
We test using clang 12 and up, and gcc 10 and up.

Any recent Python version should do.
We test using Python 3.8 and up (currently 3.8 - 3.11).

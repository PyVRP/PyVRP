[![CI](https://github.com/N-Wouda/pyvrp/actions/workflows/CI.yml/badge.svg?branch=main)](https://github.com/N-Wouda/pyvrp/actions/workflows/CI.yml)
[![codecov](https://codecov.io/gh/N-Wouda/pyvrp/branch/main/graph/badge.svg?token=G9JKIVZOHB)](https://codecov.io/gh/N-Wouda/pyvrp)

⚠️⚠️⚠️ **This package is under very heavy development - expect things to break!** ⚠️⚠️⚠️ 

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
Now we need to install all dependencies into the local environment.
We use poetry for this, but some care must be taken: by default, `poetry install` will also build and install the `pyvrp` package.
That is something we do slightly differently using our own install script, so we must pass `--no-root` to poetry to avoid it installing `pyvrp`:
```shell
poetry install --no-root
```
Then, run the `install.sh` script to build the C++ extensions:
```shell
poetry run scripts/install.sh
```
You can now verify everything went correctly by running
```shell
poetry run pytest
```
If all tests pass without errors, you have a working installation of the codebase.

### Details

We use the Meson build system to compile the C++ extensions.
Meson is configured using the `meson.build` file in the repository root. 
You should not have to touch this file often: all installation is handled via the `scripts/install.sh` script.
For deployment, we use the [`pypa/build`](https://github.com/pypa/build) frontend.
The first uses (via `poetry-core`, as defined in `[build-system]` in the `pyproject.toml` file) the `build_extension.py` file to call into `scripts/install.sh`.

In short: (for now) everything runs via `scripts/install.sh`.

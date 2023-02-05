[![CI](https://github.com/N-Wouda/pyvrp/actions/workflows/CI.yml/badge.svg?branch=main)](https://github.com/N-Wouda/pyvrp/actions/workflows/CI.yml)
[![codecov](https://codecov.io/gh/N-Wouda/pyvrp/branch/main/graph/badge.svg?token=G9JKIVZOHB)](https://codecov.io/gh/N-Wouda/pyvrp)

# PyVRP

⚠️⚠️⚠️ **This package is under very heavy development!** ⚠️⚠️⚠️ 

The `pyvrp` package is an open-source, state-of-the-art vehicle routing problem solver.

## Local installation

If you do not have poetry, get it via
```shell
pip install poetry
```
Now install all dependencies into the local environment:
```shell
poetry install
```
Then, run the `install.sh` script to build the C++ extensions:
```shell
poetry run scripts/install.sh
```
You can now verify everything went correctly by running
```shell
poetry run pytest
```

TODO:
- build.py / meson.build
- python -m build
- poetry build
- poetry publish

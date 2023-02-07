[![CI](https://github.com/N-Wouda/pyvrp/actions/workflows/CI.yml/badge.svg?branch=main)](https://github.com/N-Wouda/pyvrp/actions/workflows/CI.yml)
[![codecov](https://codecov.io/gh/N-Wouda/pyvrp/branch/main/graph/badge.svg?token=G9JKIVZOHB)](https://codecov.io/gh/N-Wouda/pyvrp)

# PyVRP

The `pyvrp` package is an open-source, state-of-the-art vehicle routing problem solver.

## Requirements
Building this package requires [Poetry](https://python-poetry.org/docs/#installation), which can be easily installed using:
```
curl -sSL https://install.python-poetry.org | python3 -
```
It is recommend to add Poetry to your path by adding the following to your `.bashrc`:
```
export PATH="$HOME/.local/bin:$PATH"
```

## Quickstart
To install dependencies, build pyvrp and solve an instance, use:
```
poetry install
scripts/build.sh
scripts/download_vrptw_instances.sh
# poetry run python -m pyvrp.solve data/vrptw/Solomon/C101.txt --seed 42
poetry run python -m pyvrp.solve data/vrptw/ORTEC/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt --seed 42 --max_runtime 10

```

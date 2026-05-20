# Benchmarks

This directory contains microbenchmarks that are executed by default during CI runs.
These ensure PyVRP does not suffer unexpected performance regressions.

## Edge-demand overhead benchmark

To measure the edge-demand feature overhead, run:

```sh
uv run pytest benchmarks/search/test_EdgeDemands.py --codspeed
```

This benchmark provides:
- `without_edge_demands`: use this on `main` for case (1), and on your feature
  branch for case (2).
- `with_edge_demands`: case (3), available on branches that support edge-demand
  matrices.

To compare saved runs locally with `pytest-benchmark` compatible options:

```sh
# On main (legacy baseline, case 1)
uv run pytest benchmarks/search/test_EdgeDemands.py \
  --benchmark-save=edge-demands-main \
  -k without_edge_demands

# On feature branch (new code, no edge demands, case 2)
uv run pytest benchmarks/search/test_EdgeDemands.py \
  --benchmark-save=edge-demands-feature-no-edge \
  --benchmark-compare=edge-demands-main \
  -k without_edge_demands

# On feature branch (new code, with edge demands, case 3)
uv run pytest benchmarks/search/test_EdgeDemands.py \
  --benchmark-save=edge-demands-feature-with-edge \
  --benchmark-compare=edge-demands-feature-no-edge \
  -k with_edge_demands
```

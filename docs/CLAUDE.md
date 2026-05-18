# Documentation Index

This file provides an overview of PyVRP's documentation structure and content. All documentation files are in reStructuredText (`.rst`) format and organized under `docs/source/`.

## Main Documentation

### `docs/source/index.rst`
Main documentation landing page. Contains the PyVRP overview, installation quick start, and the complete table of contents linking to all documentation sections. Describes supported VRP features including pickups/deliveries, heterogeneous fleets, time windows, multiple depots, reloading, optional clients, and client groups.

## Getting Started (`docs/source/setup/`)

### `docs/source/setup/installation.rst`
Installation instructions for PyVRP. Covers installing from PyPI via pip, installing from GitHub source, and setting up a local development environment with uv to run example notebooks.

### `docs/source/setup/why_pyvrp.rst`
Comparison of PyVRP with alternative VRP solvers (VROOM, jsprit, OR-Tools). Includes feature comparison table and analysis of strengths/weaknesses across five dimensions: scale, solution quality, project activity, ease of use, and ease of modification. Helps users decide if PyVRP is right for their needs.

### `docs/source/setup/getting_help.rst`
Guide for getting support. Covers submitting bug reports (with version info and code snippets), feature requests, and information about professional support services from RoutingLab including consulting, custom development, and the FastVRP API.

### `docs/source/setup/faq.rst`
Frequently asked questions about PyVRP. Covers installation issues (Windows builds, latest binaries), modelling techniques (vehicle-specific service durations, multiple time windows, open routing problems), and debugging tips (integer overflow with partial matrices).

### `docs/source/setup/concepts.rst`
Explanation of PyVRP's data model concepts. Detailed coverage of time and duration constraints for clients, depots, and vehicles, including how time windows, service durations, release times, and shift restrictions work together. Includes illustrative diagrams.

### `docs/source/setup/benchmarks.rst`
Benchmark results for PyVRP versions and competing solvers. Contains performance tables showing percentage gaps to best-known solutions across problem variants (CVRP, VRPTW, PCVRPTW, MDVRPTW, VRPB, HFVRP, MTVRPTWR). Compares PyVRP's evolution from v0.1.0 (2023) to v0.13.0 (2026) against reference solvers like HGS-CVRP and OR-Tools.

## About Us (`docs/source/about/`)

### `docs/source/about/about.rst`
PyVRP project history and mission. Traces development from 2021 fork of HGS-CVRP through competition wins (DIMACS 2022, EURO meets NeurIPS 2022) to current maintenance by RoutingLab. Includes timeline of major features added each year (2023-2026) and project mission statement.

### `docs/source/about/citing.rst`
Citation information for academic use. Provides the INFORMS Journal on Computing paper reference (Wouda, Lan, Kool 2024) in both formatted text and BibTeX format, plus link to arXiv preprint.

### `docs/source/about/funding.rst`
Information on supporting PyVRP development. Describes funding options including GitHub sponsorship, funding specific features, and using RoutingLab's products/services. Explains importance of financial support for maintenance and development.

## API Reference (`docs/source/api/`)

### `docs/source/api/pyvrp.rst`
Core API reference for the main `pyvrp` module. Documents the high-level Model interface for defining problems, IteratedLocalSearch algorithm, PenaltyManager, Result class, and core C++ types (ProblemData, Solution, Route, Client, Depot, VehicleType, CostEvaluator). Includes solve() function and parameter classes.

### `docs/source/api/search.rst`
Search methods and operators API. Documents the LocalSearch class, SearchMethod protocol, and all built-in operators (Exchange10/20/30/11/21/31/22/32/33, SwapTails, RelocateWithDepot, RemoveAdjacentDepot, RemoveOptional). Covers BinaryOperator and UnaryOperator base classes, PerturbationManager, and neighbourhood functions.

### `docs/source/api/plotting.rst`
Plotting utilities API. Documents functions for visualizing problems and solutions: plot_coordinates, plot_demands, plot_instance, plot_objectives, plot_result, plot_route_schedule, plot_runtimes, plot_solution, plot_time_windows.

### `docs/source/api/stop.rst`
Stopping criteria API. Documents the StoppingCriterion protocol and all built-in criteria: FirstFeasible, MaxIterations, MaxRuntime, MultipleCriteria, NoImprovement. Used to control when the IteratedLocalSearch terminates.

## Developing PyVRP (`docs/source/dev/`)

### `docs/source/dev/algorithm.rst`
Overview of the Iterated Local Search (ILS) algorithm. Explains ILS principles, pseudocode, and how PyVRP implements perturbation, local search, and acceptance criteria. References academic literature (Lourenço et al. 2019, Accorsi and Vigo 2021). Notes that high-level algorithm is in Python while performance-critical components are in C++.

### `docs/source/dev/benchmarking.rst`
Instructions for running benchmarks. Specifies exact benchmark configurations for each problem variant: instance sets, runtime formulas based on CPU PassMark scores, rounding functions to use. Explains how to obtain benchmark instances via git submodule and run 10-seed averages for gap calculations.

### `docs/source/dev/contributing.rst`
Comprehensive guide for contributors. Covers setting up local development environment with uv, building C++ extensions with Meson, using GitHub Codespaces, debugging mixed Python/C++ code with VSCode, profiling with perf/Instruments, and pull request requirements (tests, documentation, NumPy docstring standard). Includes licensing information (MIT).

### `docs/source/dev/glossary.rst`
Definitions of technical terms used in PyVRP. Currently defines: Concatenation scheme (for quick route segment statistics), Penalised cost (cost function with dynamic penalty terms), and Route segment (contiguous part of a route).

### `docs/source/dev/new_vrp_variants.rst`
Guidelines for extending PyVRP to support new VRP variants. Step-by-step process using prize-collecting VRP as example: adding problem data fields, updating objective/cost evaluation, modifying search operators, writing tests, and running benchmarks. Includes hints about sane defaults, binding updates, and keeping initial PRs simple. Links to example PRs for inspiration.

### `docs/source/dev/releasing.rst`
Release process documentation for maintainers. Detailed steps for regular releases (version bumping, tagging, building, PyPI deployment, Zenodo DOI, docs archiving) and patch releases (cherry-picking, temporary branches). Explains when to skip certain steps for patches vs. major/minor releases.

### `docs/source/dev/supported_vrplib_fields.rst`
Complete reference for VRPLIB format support. Documents all specifications (CAPACITY, DIMENSION, EDGE_WEIGHT_TYPE, VEHICLES, etc.) and data sections (DEMAND_SECTION, TIME_WINDOW_SECTION, PRIZE_SECTION, etc.) that PyVRP understands when reading benchmark instances. Explains heterogeneous fleet, multi-depot, and reloading-specific sections. Notes that unrecognized fields are silently ignored.

## Organization

The documentation follows a clear structure:
- **Getting started**: For users learning PyVRP
- **About us**: Project background and support
- **API reference**: Technical API documentation (auto-generated from docstrings)
- **Developing PyVRP**: For contributors and advanced users
- **Tutorials**: Jupyter notebooks in `notebooks/` (referenced but not detailed here)

Most documentation uses Sphinx autodoc to pull content from Python/C++ code, ensuring API docs stay synchronized with implementation.

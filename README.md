![PyVRP logo](docs/source/assets/images/logo.svg)

[![PyPI version](https://badge.fury.io/py/pyvrp.svg)](https://badge.fury.io/py/pyvrp)
[![CI](https://github.com/PyVRP/PyVRP/actions/workflows/CI.yml/badge.svg?branch=main)](https://github.com/PyVRP/PyVRP/actions/workflows/CI.yml)
[![DOC](https://github.com/PyVRP/PyVRP/actions/workflows/DOC.yml/badge.svg?branch=main)](https://pyvrp.org/)
[![codecov](https://codecov.io/gh/PyVRP/PyVRP/branch/main/graph/badge.svg?token=G9JKIVZOHB)](https://codecov.io/gh/PyVRP/PyVRP)

PyVRP is an open-source, state-of-the-art vehicle routing problem (VRP) solver.
It currently supports VRPs with:
- Client demands (capacitated VRP);
- Vehicles of different capacities;
- Time windows, client service durations, and release times (VRP with time windows and release times);
- Optional clients with prizes for visiting (prize collecting).

The implementation builds on Thibaut Vidal's [HGS-CVRP][8], but has been completely redesigned to be easy to use as a highly customisable Python package, while maintaining speed and state-of-the-art performance.
Users can customise various aspects of the algorithm using Python, including population management, crossover strategies, granular neighbourhoods and operator selection in the local search.
Additionally, for advanced use cases such as supporting additional VRP variants, users can build and install PyVRP directly from the source code.

PyVRP is available on the Python package index as `pyvrp`.
It may be installed in the usual way as
```
pip install pyvrp
```
This also resolves the few core dependencies PyVRP has.
The documentation is available [here][1].

> If you are new to vehicle routing or metaheuristics, you might benefit from first reading the [introduction to VRP][6] and [introduction to HGS][7] pages.

### Examples

We provide some example notebooks that show how PyVRP may be used to solve vehicle routing problems.
These include:

- A short tutorial and introduction to PyVRP's modelling interface, [here][5].
  This is a great way to get started with PyVRP.
- A notebook solving classical VRP variants, [here][4].
  In this notebook we solve several benchmark instances of the CVRP and VRPTW problems.
  We also demonstrate how to use the plotting tools available in PyVRP to visualise the instance and statistics collected during the search procedure. 
- A notebook implementing a `solve` method using PyVRP's components, [here][9].
  This notebook is a great way to dive deeper into how PyVRP works internally.

### Contributing

We are very grateful for any contributions you are willing to make. Please have
a look [here][2] to get started. If you aim to make a large change, it is
helpful to discuss the change first in a new GitHub issue. Feel free to open
one!

### Getting help

If you are looking for help, please follow the instructions [here][3].

### How to cite PyVRP

TODO


[1]: https://pyvrp.org/

[2]: https://pyvrp.org/dev/contributing.html

[3]: https://pyvrp.org/setup/getting_help.html

[4]: https://pyvrp.org/examples/basic_vrps.html

[5]: https://pyvrp.org/examples/quick_tutorial.html

[6]: https://pyvrp.org/setup/introduction_to_vrp.html

[7]: https://pyvrp.org/setup/introduction_to_hgs.html

[8]: https://github.com/vidalt/HGS-CVRP/

[9]: https://pyvrp.org/examples/using_pyvrp_components.html

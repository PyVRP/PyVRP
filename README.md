![PyVRP logo](docs/source/assets/images/PyVRP.svg)

[![PyPI version](https://badge.fury.io/py/pyvrp.svg)](https://badge.fury.io/py/pyvrp)
[![CI](https://github.com/PyVRP/PyVRP/actions/workflows/CI.yml/badge.svg?branch=main)](https://github.com/PyVRP/PyVRP/actions/workflows/CI.yml)
[![Documentation Status](https://readthedocs.org/projects/pyvrp/badge/?version=latest)](https://pyvrp.readthedocs.io/en/latest/?badge=latest)
[![codecov](https://codecov.io/gh/PyVRP/PyVRP/branch/main/graph/badge.svg?token=G9JKIVZOHB)](https://codecov.io/gh/PyVRP/PyVRP)

The `pyvrp` package is an open-source, state-of-the-art vehicle routing problem (VRP) solver.
It currently supports the capacitated VRP (CVRP), the VRP with time windows (VRPTW), and prize-collecting. 

The implementation builds on Thibaut Vidal's [HGS-CVRP][8], but has been completely redesigned to be easy to use as a highly customisable Python package, while maintaining speed and state-of-the-art performance.
Users can customise various aspects of the algorithm using Python, including population management, crossover strategies, granular neighbourhoods and operator selection in the local search.
Additionally, for advanced use cases such as supporting additional VRP variants, users can build and install `pyvrp` directly from the source code.

`pyvrp` may be installed in the usual way as
```
pip install pyvrp
```
This also resolves the few core dependencies `pyvrp` has.
The documentation is available [here][1].

> If you are new to vehicle routing or metaheuristics, you might benefit from first reading the [introduction to VRP][6] and [introduction to HGS][7] pages.

### Examples

We provide some example notebooks that show how the `pyvrp` package may be used to solve vehicle routing problems.
These include:

- The vehicle routing problem with time windows (VRPTW), [here][4].
  We solve several instances from the literature, including a large 1000 customer instance.
- The capacitated vehicle routing problem (CVRP), [here][5].
  We solve an instance with 439 customers to near optimality within 30 seconds.

### Contributing

We are very grateful for any contributions you are willing to make. Please have
a look [here][2] to get started. If you aim to make a large change, it is
helpful to discuss the change first in a new GitHub issue. Feel free to open
one!

### Getting help

If you are looking for help, please follow the instructions [here][3].

### How to cite PyVRP

TODO


[1]: https://pyvrp.readthedocs.io/en/latest/

[2]: https://pyvrp.readthedocs.io/en/latest/dev/contributing.html

[3]: https://pyvrp.readthedocs.io/en/latest/setup/getting_help.html

[4]: https://pyvrp.readthedocs.io/en/latest/examples/vrptw.html

[5]: https://pyvrp.readthedocs.io/en/latest/examples/cvrp.html

[6]: https://pyvrp.readthedocs.io/en/latest/setup/introduction_to_vrp.html

[7]: https://pyvrp.readthedocs.io/en/latest/setup/introduction_to_hgs.html

[8]: https://github.com/vidalt/HGS-CVRP/

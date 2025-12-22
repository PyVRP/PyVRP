![PyVRP logo](docs/source/assets/images/logo.svg)

[![PyPI version](https://img.shields.io/pypi/v/PyVRP?style=flat-square&label=PyPI)](https://pypi.org/project/pyvrp/)
[![CI](https://img.shields.io/github/actions/workflow/status/PyVRP/PyVRP/.github%2Fworkflows%2FCI.yml?branch=main&style=flat-square&logo=github&label=CI)](https://github.com/PyVRP/PyVRP/actions/workflows/CI.yml)
[![DOC](https://img.shields.io/github/actions/workflow/status/PyVRP/PyVRP/.github%2Fworkflows%2FDOC.yml?branch=main&style=flat-square&logo=github&label=DOC)](https://pyvrp.org/)
[![codecov](https://img.shields.io/codecov/c/github/PyVRP/PyVRP?style=flat-square&logo=codecov&label=Codecov)](https://codecov.io/gh/PyVRP/PyVRP)
[![DOI:10.1287/ijoc.2023.0055](https://img.shields.io/badge/DOI-ijoc.2023.0055-green?style=flat-square&color=blue)](https://doi.org/10.1287/ijoc.2023.0055)

PyVRP is an open-source, state-of-the-art vehicle routing problem (VRP) solver developed by [RoutingLab](https://routinglab.tech).
It currently supports VRPs with:
- Pickups and deliveries between depots and clients (capacitated VRP, VRP with simultaneous pickup and delivery, VRP with backhaul);
- Vehicles of different capacities, costs, shift durations, routing profiles, and maximum distance and duration constraints (heterogeneous fleet VRP, site-dependent VRP);
- Time windows, client service durations, and release times (VRP with time windows and release times);
- Multiple depots (multi-depot VRP);
- Reloading along routes at different reload depots (multi-trip VRP);
- Optional clients with prizes for visiting (prize collecting, team orienteering problem);
- Client groups imposing additional restrictions on multiple clients jointly (generalised VRP, VRP with multiple time windows).

PyVRP is available on the Python package index as `pyvrp`.
It may be installed in the usual way as
```
pip install pyvrp
```

The documentation is available [here][1].

> [!TIP]
> Looking for professional support? [RoutingLab](https://routinglab.tech) provides consulting, custom development, and FastVRP - a production-ready route optimisation API built on PyVRP.

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

If you are new to vehicle routing or metaheuristics, you might also benefit from reading the [introduction to VRP][6] and [introduction to ILS][7] pages.

### Getting help

Feel free to open an issue or a new discussion thread here on GitHub.
When writing your issue or discussion, please follow the instructions [here][3].
For professional support, contact us at [info@routinglab.tech](mailto:info@routinglab.tech).

### Contributing

While we are very grateful for any contributions you are willing to make, reviewing and maintaining third-party code takes a significant amount of our time.
Before you start working on your contribution, please have a look [here][2] to get started.
Make sure to discuss the change first in a GitHub issue.
Feel free to open a new one if no appropriate issue already exists!

### How to cite PyVRP

If you use PyVRP in your research, please consider citing the following paper:

> Wouda, N.A., L. Lan, and W. Kool (2024).
> PyVRP: a high-performance VRP solver package.
> _INFORMS Journal on Computing_, 36(4): 943-955.
> https://doi.org/10.1287/ijoc.2023.0055

Or, using the following BibTeX entry:

```bibtex
@article{Wouda_Lan_Kool_PyVRP_2024,
  doi = {10.1287/ijoc.2023.0055},
  url = {https://doi.org/10.1287/ijoc.2023.0055},
  year = {2024},
  volume = {36},
  number = {4},
  pages = {943--955},
  publisher = {INFORMS},
  author = {Niels A. Wouda and Leon Lan and Wouter Kool},
  title = {{PyVRP}: a high-performance {VRP} solver package},
  journal = {INFORMS Journal on Computing},
}
```

A preprint of this paper is available on [arXiv][11]. 
Since PyVRP extends [HGS-CVRP][8], please also consider citing [Vidal (2022)][10].

[1]: https://pyvrp.org/

[2]: https://pyvrp.org/dev/contributing.html

[3]: https://pyvrp.org/setup/getting_help.html

[4]: https://pyvrp.org/examples/basic_vrps.html

[5]: https://pyvrp.org/examples/quick_tutorial.html

[6]: https://pyvrp.org/setup/introduction_to_vrp.html

[7]: https://pyvrp.org/setup/introduction_to_ils.html

[8]: https://github.com/vidalt/HGS-CVRP/

[9]: https://pyvrp.org/examples/using_pyvrp_components.html

[10]: https://doi.org/10.1016/j.cor.2021.105643

[11]: https://arxiv.org/abs/2403.13795

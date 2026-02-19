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

### Tutorials

We provide many tutorials that show how to use PyVRP to solve vehicle routing problems.
The [quickstart][4] introduces PyVRP's modelling interface and is a great way to get started.

The following tutorials cover specific features in more detail:
- [Load and vehicle capacities][5]: pickups, deliveries, and multiple load dimensions.
- [Time and duration constraints][6]: time windows, service durations, release times, shifts and overtime.
- [Profiles][7]: different distances and durations for different vehicle types, and modelling access restrictions.
- [Optional clients][8]: using rewards to visit optional clients.
- [Mutually exclusive groups][9]: modelling alternative services.
- [Reloading][10]: vehicle reloading at depots during routes.

For those interested in PyVRP's underlying algorithm, see [this page][11] for a high-level description of the iterated local search algorithm, and [this notebook][12] for an implementation of the `solve` method using PyVRP's components.

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

A preprint of this paper is available on [arXiv][13].

[1]: https://pyvrp.org/

[2]: https://pyvrp.org/dev/contributing.html

[3]: https://pyvrp.org/setup/getting_help.html

[4]: https://pyvrp.org/notebooks/quick_start.html

[5]: https://pyvrp.org/notebooks/load.html

[6]: https://pyvrp.org/notebooks/duration_constraints.html

[7]: https://pyvrp.org/notebooks/profiles.html

[8]: https://pyvrp.org/notebooks/optional_clients.html

[9]: https://pyvrp.org/notebooks/mutually_exclusive_groups.html

[10]: https://pyvrp.org/notebooks/reloading.html

[11]: https://pyvrp.org/dev/algorithm.html

[12]: https://pyvrp.org/notebooks/pyvrp_implementation.html

[13]: https://arxiv.org/abs/2403.13795

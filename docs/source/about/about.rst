About PyVRP
===========

Development history
^^^^^^^^^^^^^^^^^^^

PyVRP started in 2021 as a fork of the excellent hybrid genetic search for the capacitated vehicle routing problem (`HGS-CVRP <https://github.com/vidalt/HGS-CVRP/>`_) solver of Thibaut Vidal.
This fork was expanded with support for time windows by a small team from `ORTEC <https://ortec.com/>`_ to compete in the `12th DIMACS implementation challenge dedicated to vehicle routing problems <http://dimacs.rutgers.edu/programs/challenge/vrp/>`_, where it won first place in the VRP with time windows track.
The resulting codebase was made available for the `EURO meets NeurIPS 2022 Vehicle Routing Competition <https://euro-neurips-vrp-2022.challenges.ortec.com/>`_, where it was extended by the current maintainers to include Python bindings and additional operators, to eventually again score first place on the static track.
The PyVRP name was coined in late 2022, and the project found its current home on GitHub around the same time.
Today, PyVRP is maintained by `RoutingLab <https://routinglab.tech/>`_, a start-up founded by the original PyVRP developers.

Timeline
--------

Starting in late 2022, PyVRP has seen many extensions and improvements to become the capable vehicle routing solver that it is today.
The following timeline sketches the most significant to date:

* **2023**: PyVRP is released, and adds support for multiple vehicle types, optional clients, and a high-level :class:`~pyvrp.Model.Model` interface;
* **2024**: PyVRP adds support for multiple depots, simultaneous pickups and deliveries, mutually exclusive groups, multiple distance and duration matrices through the profile mechanism, and multiple load dimensions;
* **2025**: PyVRP adds support for routing problems with intermediate reloading at depots;
* **2026**: PyVRP solves instances of many thousands of clients much better than before.

Mission
^^^^^^^

PyVRP aims to be the go-to VRP solver in the Python ecosystem, by combining excellent performance and ease-of-use.
Additionally, we have the broader goal of making route optimisation available to every organisation.

.. figure:: assets/images/logo.svg
   :alt: PyVRP logo
   :figwidth: 100%

PyVRP is an open-source, state-of-the-art vehicle routing problem (VRP) solver maintained by `RoutingLab <https://routinglab.tech>`_.
It currently supports VRPs with:

* Pickups and deliveries between depots and clients (capacitated VRP, VRP with simultaneous pickup and delivery, VRP with backhaul);
* Vehicles of different capacities, costs, shift durations, routing profiles, and maximum distance and duration constraints (heterogeneous fleet VRP, site-dependent VRP);
* Time windows, client service durations, and release times (VRP with time windows and release times);
* Multiple depots (multi-depot VRP);
* Reloading along routes at different reload depots (multi-trip VRP);
* Optional clients with prizes for visiting (prize collecting, team orienteering problem);
* Client groups imposing additional restrictions on multiple clients jointly (generalised VRP, VRP with multiple time windows).

The PyVRP package comes with pre-compiled binaries for Windows, Mac OS and Linux, and can thus be easily installed without requiring local compilation.
It can be installed through *pip* via

.. code-block:: shell

   pip install pyvrp

.. tip::

   Looking for professional support? `RoutingLab <https://routinglab.tech>`_ provides consulting, custom development, and FastVRP - a production-ready route optimisation API built on PyVRP.

Contents
--------

.. toctree::
   :maxdepth: 1
   :caption: Getting started

   setup/introduction_to_vrp
   setup/introduction_to_hgs
   setup/installation
   setup/getting_help
   setup/faq
   setup/concepts
   setup/benchmarks
   setup/citing

.. toctree::
   :maxdepth: 1
   :caption: Examples

   examples/quick_tutorial
   examples/basic_vrps
   examples/using_pyvrp_components

.. toctree::
   :maxdepth: 1
   :caption: API reference

   api/pyvrp
   api/crossover
   api/diversity
   api/repair
   api/search
   api/plotting
   api/stop

.. toctree::
   :maxdepth: 1
   :caption: Developing PyVRP

   dev/benchmarking
   dev/contributing
   dev/releasing
   dev/supported_vrplib_fields
   dev/new_vrp_variants
   dev/glossary

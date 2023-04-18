PyVRP
=====

`pyvrp` is a flexible, extensible, and well-documented Python package for solving Vehicle Routing Problems (VRPs) and generating state-of-the-art results on various VRP variants.
The package provides an implementation of the Hybrid Genetic Search (HGS) algorithm for VRPs, catering to both practitioners looking to solve real-world problems and researchers seeking a starting point or strong baseline for improving the state of the art.
`pyvrp` offers the flexibility to customize the overall algorithm using Python and the provided HGS implementation serves as just one example of a large family of algorithms that can be built using the building blocks of this package.
Users can create custom operators or even implement entirely new algorithms using the provided framework.

In particular, `pyvrp` provides the following features:
- A complete implementation of the HGS algorithm to solve the Capacitated Vehicle Routing Problem (CVRP) and Vehicle Routing Problem with Time Windows (VRPTW)
- A variety of operators, such as crossover, local search, destroy and repair operators

``pyvrp`` may be installed in the usual way as

.. code-block:: shell

   pip install pyvrp

.. hint::

    If you are new to vehicle routing or metaheuristics, you might benefit from first reading :doc:`introduction to VRP <setup/introduction_to_vrp>` and :doc:`introduction to HGS for VRP <setup/introduction_to_hgs>`.
    To set up an installation from source, or to run the examples listed below yourself, please have a look at the :doc:`installation instructions <setup/installation>`.

.. toctree::
   :maxdepth: 1
   :caption: Getting started

   setup/introduction_to_hgs
   setup/installation
   setup/getting_help
   setup/benchmarks

.. toctree::
   :maxdepth: 1
   :caption: Examples

   examples/vrptw
   examples/cvrp

.. toctree::
   :maxdepth: 1
   :caption: API reference

   api/pyvrp
   api/crossover
   api/diagnostics
   api/diversity
   api/educate
   api/plotting
   api/stop

.. toctree::
   :maxdepth: 1
   :caption: Developing PyVRP

   dev/contributing
   dev/benchmarking

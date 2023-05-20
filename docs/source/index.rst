.. image:: images/pyvrp_rtd.png

PyVRP is a Python package that offers a high-performance implementation of the hybrid genetic search algorithm for vehicle routing problems (VRPs).
PyVRP currently supports two well-known VRP variants: the Capacitated VRP (CVRP) and the Vehicle Routing Problem with Time Windows (VRPTW). 
The implementation is inspired by `HGS-CVRP <https://github.com/vidalt/HGS-CVRP/>`_, but has added support for time windows and has been completely redesigned to be easy to use as a highly customisable Python package, while maintaining speed and state-of-the-art performance.

This allows users to directly solve VRP instances, or implement variants of the HGS algorithm using Python, inspired by the examples in this documentation. 
Users can customise various aspects of the algorithm using Python, including population management, crossover strategies, granular neighbourhoods and operator selection in the local search.
Additionally, for advanced use cases such as supporting additional VRP variants, users can build and install PyVRP directly from the source code.

The PyVRP package comes with pre-compiled binaries for Windows, Mac OS and Linux, and can thus be easily installed without requiring local compilation.
It can be installed through *pip* via

.. code-block:: shell

   pip install pyvrp

.. hint::

    If you are new to metaheuristics or vehicle routing, you might benefit from first reading the :doc:`introduction to HGS for VRP <setup/introduction_to_hgs>`.
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
   api/diversity
   api/educate
   api/plotting
   api/stop

.. toctree::
   :maxdepth: 1
   :caption: Developing PyVRP

   dev/contributing
   dev/benchmarking

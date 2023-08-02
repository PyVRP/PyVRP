Benchmarks
==========

This page lists benchmark results obtained by recent versions of the PyVRP package and reference VRP solvers.
See the :doc:`benchmarking instructions <../dev/benchmarking>` for how these benchmarks are run. 

PyVRP
-----

The table below contains the benchmark results obtained by each new significant version of PyVRP.
The reported values represent the percentage gap to the best-known solutions at the time of the benchmark, averaged over ten seeds.
These best-known solutions are tracked in the ``PyVRP/Instances`` repository - see the :doc:`benchmarking instructions <../dev/benchmarking>` for details.
The configuration files used for benchmarking are ``configs/cvrp.toml`` (for CVRP) and ``configs/vrptw.toml`` (for VRPTW and PC-VRPTW).

.. list-table::
   :header-rows: 1

   * - Date
     - Version
     - CVRP
     - VRPTW
     - PC-VRPTW
   * - 1 August 2023
     - `0.5.0 <https://github.com/PyVRP/PyVRP/tree/d4799a810a8cf7d16ea2c8871204bdfb3a896d06>`_
     - 0.22%
     - 0.40%
     - 0.23%
   * - 9 July 2023
     - `0.4.2 <https://github.com/PyVRP/PyVRP/tree/f934e0da184dd0bdbd4d83f72ec98b7ef51cd8da>`_
     - 0.18%
     - 0.43%
     - 0.23%
   * - 20 May 2023
     - `0.3.0 <https://github.com/PyVRP/PyVRP/tree/4632ce97cedbc9d58216c2bec43cd679eb1d21c9>`_
     - 0.25%
     - 0.43%
     - 0.21%
   * - 13 May 2023
     - `0.2.0 <https://github.com/PyVRP/PyVRP/tree/3784f03fa3b6777613fb0bc8cedeac5ad372cfe4>`_
     - 0.25%
     - 0.43%
     -
   * - 28 February 2023
     - `0.1.0 <https://github.com/PyVRP/PyVRP/tree/e1b1ac72bc1246cc51d252bf72df71fc43dc422b>`_
     - 0.22%
     - 0.45%
     -


Reference VRP solvers
---------------------

The table below contains the benchmark results obtained by reference VRP solvers.
The reported values represent the average gaps to the best-known solutions over ten seeds.

.. list-table::
   :header-rows: 1

   * - Date
     - Name
     - CVRP
     - VRPTW
     - PC-VRPTW
   * - 28 February 2023
     - `HGS-CVRP <https://github.com/vidalt/HGS-CVRP>`_
     - 0.11%
     -
     -
   * - 28 February 2023
     - `HGS-DIMACS <https://github.com/ortec/euro-neurips-vrp-2022-quickstart#baseline-solver-hybrid-genetic-search-hgs>`_
     -
     - 0.32%
     -

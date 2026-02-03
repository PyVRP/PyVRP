Benchmarking
============

To run a benchmark, one can use the command line interface PyVRP provides.
After installation, it is available as ``pyvrp`` from the command line.
To find out about available options, run

.. code-block:: shell

   pyvrp --help

We use the following instances and configurations to benchmark PyVRP:

* For the CVRP, we use `the X-instances of Uchoa <https://github.com/PyVRP/Instances/tree/main/CVRP>`_.
  Each instance has :math:`n` clients; the runtimes are computed as :math:`2.4 n` seconds on a reference CPU with PassMark score 2183.
  Thus, an instance with 100 clients is ran for 240 seconds, assuming a CPU with PassMark score 2183.
  For this benchmark, the ``round`` rounding function should be used.

* For the VRPTW, we use `the Gehring and Homberger instances <https://github.com/PyVRP/Instances/tree/main/VRPTW#vrptw>`_, particularly those with 1000 clients.
  These instances are each run for two hours on a reference CPU with PassMark score 2000.
  For this benchmark, the ``dimacs`` rounding function should be used.

* For the PCVRPTW, we use `the modified Gehring and Homberger instances <https://github.com/PyVRP/Instances/tree/main/PCVRPTW#pcvrptw>`_ with 1000 clients.
  These instances are each run for two hours on a reference CPU with PassMark score 2000.
  For this benchmark, the ``dimacs`` rounding function should be used.

* For the MDVRPTW, we use `the large MDVRPTW instances of Vidal et al. (2013) <https://github.com/PyVRP/Instances/tree/main/MDVRPTW#mdvrptw>`_.
  These instances range in size from 360 to 960 clients.
  The instances are each run for one hour on a reference CPU with PassMark score 2000.
  For this benchmark, the ``exact`` rounding function should be used.

* For the VRPB, we use `the 90 largest VRPB instances of Queiroga et al. (2020) <https://github.com/PyVRP/Instances/tree/main/VRPB#vrpb>`_.
  These instances range in size from 523 to 1000 clients.
  Each instance has :math:`n` clients; the runtimes are computed as :math:`2.4 n` seconds on a reference CPU with PassMark score 2000.
  Thus, an instance with 100 clients is ran for 240 seconds, assuming a CPU with PassMark score 2000.
  For this benchmark, the ``round`` rounding function should be used.

* For the HFVRP, we use `the HFVRP instances of Pessoa et al. (2018) <https://github.com/PyVRP/Instances/tree/main/HFVRP#hfvrp>`_.
  These instances range in size from 100 to 1000 clients, and are based on the X instances for CVRP.
  Each instance has :math:`n` clients; the runtimes are computed as :math:`2.4 n` seconds on a reference CPU with PassMark score 2000.
  Thus, an instance with 100 clients is ran for 240 seconds, assuming a CPU with PassMark score 2000.
  For this benchmark, the ``exact`` rounding function should be used.

* For the MTVRPTWR, we use `the instances of Yang (2023) and Yang (2024) <https://github.com/PyVRP/Instances/tree/main/MTVRPTWR#mtvrptwr>`_.
  These instances range in size from 100 to 200 clients and are based on the Solomon and Gehring & Homberger VRPTW instances.
  The instances are each run for ten minutes on a reference CPU with PassMark score 2000.
  For this benchmark, the ``dimacs`` rounding function should be used.

The time limit should be scaled by the PassMark score of your CPU.
Each instance is run ten times with different seeds.
Each run is performed on a single core.
For each instance, we take the average objectives of these ten runs with different seeds.
These are compared with the best-known solutions tracked in the ``PyVRP/Instances`` `repository <https://github.com/PyVRP/Instances>`_ to compute gaps.

To get the benchmark instance sets quickly, the ``PyVRP/Instances`` repository may be initialised as a submodule:

.. code-block:: shell

   git submodule init instances

After running this command, the instances will be available in ``instances/``.

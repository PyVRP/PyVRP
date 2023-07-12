Benchmarking
============

To run a benchmark, one can use the command line interface PyVRP provides.
After installation, it is available as ``pyvrp`` from the command line.
To find out about available options, run

.. code-block:: shell

   pyvrp --help

To benchmark PyVRP, we mostly use instances from `CRVPLIB <http://vrp.atd-lab.inf.puc-rio.br/index.php/en/>`_ instances.
Particularly:

* For the CVRP, we use `the X-instances of Uchoa <http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-X.tgz>`_.
  Each instance has :math:`n` clients; the runtimes are computed as :math:`2.4 n` seconds on a reference CPU with PassMark score 2183.
  Thus, an instance with 100 clients is ran for 240 seconds, assuming a CPU with PassMark score 2183.

* For the VRPTW, we use `the Gehring and Homberger instances <http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-HG.tgz>`_, particularly those with 1000 customers.
  These instances are each run for two hours on a reference CPU with PassMark score 2000.

* For the PC-VRPTW, we use `the modified Gehring and Homberger instances <https://github.com/PyVRP/Instances/tree/main/PC-VRPTW#pc-vrptw>`_ with 1000 customers.
  These instances are each run for two hours on a reference CPU with PassMark score 2000.

The time limit should be scaled by the PassMark score of your CPU.
Each instance is run ten times with different seeds.
Each run is performed on a single core.
For each instance, we take the average objectives of these ten runs with different seeds.
These are compared with the best-known solutions tracked in the ``PyVRP/Instances`` `repository <https://github.com/PyVRP/Instances>`_ to compute gaps.

To get the benchmark instance sets quickly, the ``PyVRP/Instances`` repository may be initialised as a submodule:

.. code-block:: shell

   git submodule init instances

After running this command, the instances will be available in ``instances/``.

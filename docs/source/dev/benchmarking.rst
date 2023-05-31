Benchmarking
============

To run a benchmark, one can use the command line interface ``PyVRP`` provides.
After installation, it is available as ``pyvrp`` from the command line.
To find out about available options, run

.. code-block:: shell

   pyvrp --help

To benchmark ``pyvrp``, we mostly use instances from `CRVPLIB <http://vrp.atd-lab.inf.puc-rio.br/index.php/en/>`_ instances.
Particularly:

* For the VRPTW, we use `the Gehring and Homberger instances <http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-HG.tgz>`_, particularly those with 1000 customers.
  These instances are each run for two hours (on a reference CPU with PassMark score 2000; the time limit should be scaled by the score of your CPU).

* For the CVRP, we use `the X-instances of Uchoa <http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-X.tgz>`_.
  Each instance has :math:`n` clients; the runtimes are computed as :math:`2.4 n` seconds (on a reference CPU with PassMark score 2183; the time limit should be scaled by the score of your CPU).
  Thus, an instance with 100 clients is ran for 240 seconds, assuming a CPU with PassMark score 2183. 

Each instance is run ten times with different seeds.
Each run is performed on a single core.
For each instance, we take the average objectives/gaps of these ten runs with different seeds.

To get the benchmark instance sets quickly, the ``PyVRP/Instances`` repository may be initialised as a submodule:

.. code-block:: shell

   git submodule init instances

After running this command, the instances will be available in ``instances/``.

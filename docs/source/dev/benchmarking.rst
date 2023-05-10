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

* For the CVRP, we use `the X-instances of Uchoa <http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-X.tgz>`_.

To get these instance sets quickly, the ``PyVRP/Instances`` repository may be initialised as a submodule:

.. code-block:: shell

   git submodule init instances

After running this comamnd, the instances will be available in ``instances/``.

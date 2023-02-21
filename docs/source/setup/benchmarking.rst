Benchmarking
============

To run a benchmark, one can use the command line interface ``PyVRP`` provides.
After installation, it is available as ``pyvrp`` from the command line.
To find out about available options, run

.. code-block:: shell

   pyvrp --help

To benchmark ``pyvrp``, we use the following `CRVPLIB <http://vrp.atd-lab.inf.puc-rio.br/index.php/en/>`_ instances:

* For the VRPTW, we use `the Gehring and Homberger instances <http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-HG.tgz>`_, particularly those with 1000 customers.

* For the CVRP, we use `the X-instances of Uchoa <http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-X.tgz>`_.

To get both instance sets quickly, the following script can be used to download and unpack them:

.. code-block:: shell

   echo "Downloading GH instances";
   wget "http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-HG.tgz";
   tar -xvf "Vrp-Set-HG.tgz" --strip-components=1;
   mv Vrp-Set-HG GH;
   rm "Vrp-Set-HG.tgz";
   
   echo "Downloading X instances";
   wget "http://vrp.atd-lab.inf.puc-rio.br/media/com_vrp/instances/Vrp-Set-X.tgz";
   tar -xvf "Vrp-Set-X.tgz" --strip-components=1;
   rm "Vrp-Set-X.tgz";

These commands create two directories, ``GH/`` and ``X/``, which contain the downloaded problem instance files and the current best known solutions.

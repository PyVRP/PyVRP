Benchmarks
==========

This page lists benchmark results obtained by recent versions of the PyVRP package.
See the :doc:`benchmarking instructions <../dev/benchmarking>` for how these benchmarks are run. 

v0.1.0 (1 March 2023)
---------------------

This benchmark concerns commit `e1b1ac7 <https://github.com/N-Wouda/PyVRP/tree/e1b1ac72bc1246cc51d252bf72df71fc43dc422b>`_, the version used to obtain the results of the initial submission.
The gaps below are computed against best-known solutions obtained from CVRPLIB on 28 February 2023.
The parameter values were changed somewhat from the defaults in that commit.
The relevant configuration files can be found in `3c26fab <https://github.com/N-Wouda/PyVRP/tree/3c26fab44ba612bae4a225daa099aefc1e618d9e>`_, under ``configs/``.

CVRP
^^^^

Average gaps over ten seeds on the X-instances:

.. code-block::

                    PyVRP  HGS-CVRP

          Mean gap  0.22%     0.11%
   Gap of the mean  0.30%     0.16%

VRPTW
^^^^^

Average gaps over ten seeds on the Gehring and Homberger instances with 1000 customers:

.. code-block::

                    PyVRP  HGS-DIMACS

          Mean gap  0.45%       0.32%
   Gap of the mean  0.54%       0.37%

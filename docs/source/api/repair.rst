.. module:: pyvrp.repair
   :synopsis: Repair operators


Repair operators
================

The :mod:`pyvrp.repair` module provides operators that are responsible for repairing a :class:`~pyvrp._pyvrp.Solution` after destruction in a large neighbourhood search (LNS) setting.
These operators take a given list of routes which represent the solution to repair, and insert the unplanned clients into these routes.
To allow fine-grained control over the number of routes and vehicles used, no new routes are created: clients are only inserted into the given routes.

.. automodule:: pyvrp.repair._repair

   .. autofunction:: greedy_repair

   .. autofunction:: nearest_route_insert

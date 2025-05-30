.. module:: pyvrp.perturb
   :synopsis: Perturb


Perturbation methods
====================

The :mod:`pyvrp.perturb` module contains classes responsible for perturbing a solution.
This step is important for escaping local optima.
PyVRP currently provides a :class:`DestroyRepair` method.

All perturbation methods implement the :class:`PerturbationMethod` protocol.

.. automodule:: pyvrp.perturb.PerturbationMethod

   .. autoclass:: PerturbationMethod
      :members:
      :special-members: __call___

.. automodule:: pyvrp.perturb.DestroyRepair

   .. autoclass:: pyvrp.perturb.DestroyRepair
      :members:
      :special-members: __call___



Operators
---------

.. automodule:: pyvrp.perturb._perturb

   .. autofunction:: neighbour_removal

   .. autofunction:: greedy_repair

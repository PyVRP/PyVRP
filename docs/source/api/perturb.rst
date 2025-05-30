.. module:: pyvrp.perturb
   :synopsis: Perturb


Perturbation methods
====================

The :mod:`pyvrp.perturb` module contains classes responsible for perturbing a solution.
Perturbation methods strategically modify solutions to help escape local optima in the subsequent search step.
PyVRP currently provides a :class:`DestroyRepair` method.

All perturbation methods implement the :class:`PerturbationMethod` protocol.

.. automodule:: pyvrp.perturb.PerturbationMethod

   .. autoclass:: PerturbationMethod
      :members:
      :special-members: __call___

.. automodule:: pyvrp.perturb._perturb
   :noindex:

   .. autoclass:: DestroyRepair
      :members:
      :special-members: __call___


Destroy operators
-----------------

Instances of these operators can be added to the :class:`~pyvrp.perturb._perturb.DestroyRepair` object via the :meth:`~pyvrp.perturb._perturb.DestroyRepair.add_destroy_operator` method.
Each destroy operator inherits from :class:`~pyvrp.perturb._perturb.DestroyOperator`.
As a convenience, the :mod:`pyvrp.perturb` module makes all these operators available as ``DESTROY_OPERATORS``:

.. code-block:: python

   from pyvrp.perturb import DESTROY_OPERATORS


.. automodule:: pyvrp.perturb._perturb
   :noindex:

   .. autoclass:: DestroyOperator

   .. autoclass:: NeighbourRemoval
      :exclude-members: __call__


Repair operators
----------------

Instances of these operators can be added to the :class:`~pyvrp.perturb._perturb.DestroyRepair` object via the :meth:`~pyvrp.perturb._perturb.DestroyRepair.add_repair_operator` method.
Each repair operator inherits from :class:`~pyvrp.perturb._perturb.RepairOperator`.
As a convenience, the :mod:`pyvrp.perturb` module makes all these operators available as ``REPAIR_OPERATORS``:

.. code-block:: python

   from pyvrp.perturb import REPAIR_OPERATORS


.. automodule:: pyvrp.perturb._perturb
   :noindex:

   .. autoclass:: RepairOperator

   .. autoclass:: GreedyRepair
      :exclude-members: __call__

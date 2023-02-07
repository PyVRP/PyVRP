.. module:: pyvrp.stop
   :synopsis: Stopping criteria


Stopping criteria
=================

The :mod:`pyvrp.stop` module contains the various stopping criteria the ``pyvrp`` package ships with.
These can be used to stop the :class:`~pyvrp.GeneticAlgorithm.GeneticAlgorithm`'s' search whenever some criterion is met: for example, when some maximum number of iterations or run-time is exceeded.

All stopping criteria implement the :class:`~pyvrp.stop.StoppingCriterion.StoppingCriterion` protocol.

.. automodule:: pyvrp.stop.StoppingCriterion
   :members:

.. automodule:: pyvrp.stop.MaxIterations
   :members:

.. automodule:: pyvrp.stop.MaxRuntime
   :members:

.. automodule:: pyvrp.stop.NoImprovement
   :members:

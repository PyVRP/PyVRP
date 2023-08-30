.. module:: pyvrp.stop
   :synopsis: Stopping criteria


Stopping criteria
=================

The :mod:`pyvrp.stop` module contains the various stopping criteria the ``pyvrp`` package ships with.
These can be used to stop the :class:`~pyvrp.GeneticAlgorithm.GeneticAlgorithm`'s' search whenever some criterion is met: for example, when some maximum number of iterations or run-time is exceeded.

All stopping criteria implement the :class:`~pyvrp.stop.StoppingCriterion.StoppingCriterion` protocol.

.. automodule:: pyvrp.stop.StoppingCriterion

   .. autoclass:: StoppingCriterion
      :members:
      :special-members: __call__

.. automodule:: pyvrp.stop.MaxIterations

   .. autoclass:: MaxIterations
      :members:

.. automodule:: pyvrp.stop.MaxRuntime

   .. autoclass:: MaxRuntime
      :members:

.. automodule:: pyvrp.stop.NoImprovement

   .. autoclass:: NoImprovement
      :members:

.. automodule:: pyvrp.stop.TimedNoImprovement

   .. autoclass:: TimedNoImprovement
      :members:

.. module:: pyvrp.accept
   :synopsis: Acceptance criteria


Acceptance criteria
===================

The :mod:`pyvrp.accept` module contains various acceptance criteria that determine whether to accept or reject a candidate solution in the iterated local search algorithm.

An effective acceptance criterion finds balance between accepting worse solutions for exploration and keeping good solutions for exploitation.
This helps the algorithm avoid getting stuck in local optima while still generally moving toward better solutions over time.

All acceptance criteria implement the :class:`~pyvrp.accept.AcceptanceCriterion.AcceptanceCriterion` protocol.

.. automodule:: pyvrp.accept.AcceptanceCriterion

   .. autoclass:: AcceptanceCriterion
      :members:
      :special-members: __call__

.. automodule:: pyvrp.accept.MovingBestAverageThreshold

   .. autoclass:: MovingBestAverageThreshold
      :members:

   .. autoclass:: MovingBestAverageThresholdParams
      :members:

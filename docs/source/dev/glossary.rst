Glossary
========

This is a glossary of terms we use regularly, but are not defined anywhere else.
Feel free to suggest new entries!

.. glossary::
   :sorted:

   Concatenation scheme

      These are used in the C++ part of the codebase to quickly compute statistics of a :term:`Route segment`.
      See the ``DistanceSegment`` or ``DurationSegment`` in the codebase for examples.

   Penalised cost

      The cost function :meth:`~pyvrp._pyvrp.CostEvaluator.cost` plus penalty terms for infeasibilities.
      The penalty terms vary dynamically based on recent feasibility; this is handled by the :class:`~pyvrp.PenaltyManager`.

   Route segment

      A contiguous part of a route, that is, one or more customers visited in sequence. 

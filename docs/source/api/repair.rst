.. module:: pyvrp.repair
   :synopsis: Repair operators


Repair operators
================

The :mod:`pyvrp.repair` module provides operators that are responsible for repairing a :class:`~pyvrp._pyvrp.Solution` after destruction in a large neighbourhood search (LNS) setting.

.. automodule:: pyvrp.repair._repair

   .. function:: greedy_repair(solution: Solution, unplanned: List[int], data: ProblemData, cost_evaluator: CostEvaluator) -> Solution
                 greedy_repair(routes: List[Route], unplanned: List[int], data: ProblemData, cost_evaluator: CostEvaluator) -> Solution

      Greedy repair operator. This operator inserts each client in the list of
      unplanned clients into the solution. It does so by evaluating all possible
      moves and applying the best one for each client, resulting in a quadratic
      runtime.

      :raises: ValueError: When the solution is empty but the list of unplanned clients is not.

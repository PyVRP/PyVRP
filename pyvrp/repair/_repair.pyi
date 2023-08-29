from typing import List, overload

from pyvrp import CostEvaluator, ProblemData, Route, Solution

@overload
def greedy_repair(
    solution: Solution,
    unplanned: List[int],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution: ...
@overload
def greedy_repair(
    routes: List[Route],
    unplanned: List[int],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution: ...

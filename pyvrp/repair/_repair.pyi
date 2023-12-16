from typing import overload

from pyvrp import CostEvaluator, ProblemData, Route, Solution

@overload
def greedy_repair(
    solution: Solution,
    unplanned: list[int],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution: ...
@overload
def greedy_repair(
    routes: list[Route],
    unplanned: list[int],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution: ...
def nearest_route_insert(
    routes: list[Route],
    unplanned: list[int],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution: ...

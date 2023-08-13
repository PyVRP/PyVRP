from typing import List, overload

from pyvrp import CostEvaluator, DynamicBitset, ProblemData, Route, Solution

@overload
def greedy_repair(
    solution: Solution,
    unplanned: DynamicBitset,
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution: ...
@overload
def greedy_repair(
    routes: List[Route],
    unplanned: DynamicBitset,
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution: ...

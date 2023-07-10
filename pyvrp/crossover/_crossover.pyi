from typing import Tuple

from pyvrp import CostEvaluator, ProblemData, Solution

def selective_route_exchange(
    parents: Tuple[Solution, Solution],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
    start_indices: Tuple[int, int],
    num_moved_routes: int,
) -> Solution: ...

from pyvrp import CostEvaluator, ProblemData, Solution

def selective_route_exchange(
    parents: tuple[Solution, Solution],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
    start_indices: tuple[int, int],
    num_moved_routes: int,
) -> Solution: ...

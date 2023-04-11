from typing import Tuple

from pyvrp._CostEvaluator import CostEvaluator
from pyvrp._Individual import Individual
from pyvrp._ProblemData import ProblemData

def selective_route_exchange(
    parents: Tuple[Individual, Individual],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
    start_indices: Tuple[int, int],
    num_moved_routes: int,
) -> Individual: ...

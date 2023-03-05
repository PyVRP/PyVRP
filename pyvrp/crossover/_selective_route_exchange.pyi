from typing import Tuple

from pyvrp._Individual import Individual
from pyvrp._PenaltyManager import PenaltyManager
from pyvrp._ProblemData import ProblemData

def selective_route_exchange(
    parents: Tuple[Individual, Individual],
    data: ProblemData,
    penalty_manager: PenaltyManager,
    start_a: int,
    start_b: int,
    num_moved_routes: int,
) -> Individual: ...

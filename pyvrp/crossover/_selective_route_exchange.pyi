from typing import Tuple

from pyvrp import Individual, PenaltyManager, ProblemData, XorShift128

def selective_route_exchange(
    parents: Tuple[Individual, Individual],
    data: ProblemData,
    penalty_manager: PenaltyManager,
    rng: XorShift128,
) -> Individual: ...

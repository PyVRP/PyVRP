import random as random

from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Route,
    Solution,
)
from pyvrp.repair import greedy_repair


def greedy(
    data: ProblemData,
    solution: Solution,
    cost_eval: CostEvaluator,
    rng: RandomNumberGenerator,
    neighbours,
) -> Solution:
    """
    Small wrapper around ``greedy_repair`` to have Solution's as input and
    output.
    """
    visited = {
        client for route in solution.routes() for client in route.visits()
    }
    clients = range(data.num_depots, data.num_locations)
    unplanned = [idx for idx in clients if idx not in visited]
    random.shuffle(unplanned)

    num_routes_left = data.num_vehicles - len(solution.routes())
    empty = [Route(data, [], 0) for _ in range(num_routes_left)]

    routes = greedy_repair(
        solution.routes() + empty, unplanned, data, cost_eval, neighbours
    )
    not_empty = [route for route in routes if route.visits()]
    return Solution(data, not_empty)

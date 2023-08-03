from typing import Tuple

from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)

from ._crossover import selective_route_exchange as _srex


def selective_route_exchange(
    parents: Tuple[Solution, Solution],
    data: ProblemData,
    cost_evaluator: CostEvaluator,
    rng: RandomNumberGenerator,
) -> Solution:
    """
    This crossover operator due to Nagata and Kobayashi (2010) combines routes
    from both parents to generate a new offspring solution. It does this by
    carefully selecting routes from the second parent that could be exchanged
    with routes from the first parent. After exchanging these routes, the
    resulting offspring solution is repaired using a greedy repair strategy.

    Parameters
    ----------
    parents
        The two parent solutions to create an offspring from.
    data
        The problem instance.
    cost_evaluator
        The cost evaluator to be used during the greedy repair step.
    rng
        The random number generator to use.

    Returns
    -------
    Solution
        A new offspring.

    References
    ----------
    .. [1] Nagata, Y., & Kobayashi, S. (2010). A Memetic Algorithm for the
           Pickup and Delivery Problem with Time Windows Using Selective Route
           Exchange Crossover. *Parallel Problem Solving from Nature*, PPSN XI,
           536 - 545.
    """
    first, second = parents

    if first.num_clients() == 0:
        return second

    if second.num_clients() == 0:
        return first

    idx1 = rng.randint(first.num_routes())
    idx2 = idx1 if idx1 < second.num_routes() else 0
    max_routes_to_move = min(first.num_routes(), second.num_routes())
    num_routes_to_move = rng.randint(max_routes_to_move) + 1

    return _srex(
        parents, data, cost_evaluator, (idx1, idx2), num_routes_to_move
    )

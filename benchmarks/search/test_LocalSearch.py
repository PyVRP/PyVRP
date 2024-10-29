import pytest

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution, read
from pyvrp.search import (
    NODE_OPERATORS,
    ROUTE_OPERATORS,
    LocalSearch,
    compute_neighbours,
)


@pytest.mark.parametrize("seed", range(5))
def test_RC208_all_operators(seed: int, benchmark):
    """
    Tests the local search (with all default operators) on a VRPTW instance.
    """
    data = read("tests/data/RC208.vrp")
    rng = RandomNumberGenerator(seed=seed)
    ls = LocalSearch(data, rng, compute_neighbours(data))

    for node_op in NODE_OPERATORS:
        ls.add_node_operator(node_op(data))

    for route_op in ROUTE_OPERATORS:
        ls.add_route_operator(route_op(data))

    sol = Solution.make_random(data, rng)
    cost_evaluator = CostEvaluator(20, 6, 6)
    benchmark(ls, sol, cost_evaluator)

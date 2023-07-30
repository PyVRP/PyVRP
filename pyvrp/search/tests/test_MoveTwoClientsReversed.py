from numpy.testing import assert_
from pytest import mark

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import (
    LocalSearch,
    MoveTwoClientsReversed,
    NeighbourhoodParams,
    compute_neighbours,
)
from pyvrp.tests.helpers import read


@mark.parametrize("seed", list(range(50)))
def test_single_route_OkSmall(seed):
    """
    This test checks that MoveTwoClientsReversed properly solves the small
    instance where we know what is going on.
    """
    read("data/OkSmall.txt")
    RandomNumberGenerator(seed)

    # No capacity penalty so we can visit all clients using a single route.
    CostEvaluator(0, 10000)

    # TODO rewrite this test


@mark.parametrize("seed", [2643, 2742, 2941, 3457, 4299, 4497, 6178, 6434])
def test_RC208_instance(seed: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=seed)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(MoveTwoClientsReversed(data))

    single_route = list(range(1, data.num_clients + 1))
    sol = Solution(data, [single_route])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)

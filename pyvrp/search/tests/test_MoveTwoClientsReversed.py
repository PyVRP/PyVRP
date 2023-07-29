from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import (
    LocalSearch,
    MoveTwoClientsReversed,
    NeighbourhoodParams,
    compute_neighbours,
)
from pyvrp.tests.helpers import read


def test_single_route_OkSmall():
    """
    This test checks that MoveTwoClientsReversed properly solves the small
    instance where we know what is going on.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(MoveTwoClientsReversed(data))

    sol = Solution(data, [[1, 4, 2, 3]])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    assert_equal(improved_sol.num_routes(), 1)
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)

    # (2, 3) was inserted after 1 as 1 -> 3 -> 2 -> 4. Then (1, 3) got inserted
    # after 4 as 2 -> 4 -> 3 -> 1.
    expected = Solution(data, [[2, 4, 3, 1]])
    assert_equal(improved_sol, expected)

    # These two-route solutions can all be created by MoveTwoClientsReversed
    # from the returned solution. So they must have a cost that's at best equal
    # to the returned solution's cost.
    for routes in ([[3, 1], [4, 2]], [[2, 1], [3, 4]], [[2, 4], [1, 3]]):
        other = Solution(data, routes)
        improved_cost = cost_evaluator.penalised_cost(improved_sol)
        other_cost = cost_evaluator.penalised_cost(other)
        assert_(improved_cost <= other_cost)


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

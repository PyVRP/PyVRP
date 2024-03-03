import pytest
from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import (
    LocalSearch,
    NeighbourhoodParams,
    ReverseSegment,
    compute_neighbours,
)
from pyvrp.search._search import Node, Route


def test_reverse_segment(ok_small):
    """
    This test checks that a reverse segment move is correctly evaluated and
    applied on a single-route solution to the OkSmall instance.
    """
    nodes = [Node(loc=loc) for loc in range(ok_small.num_locations)]

    # Current route is 4 -> 1 -> 2 -> 3.
    route = Route(ok_small, idx=0, vehicle_type=0)
    route.append(nodes[4])
    route.append(nodes[1])
    route.append(nodes[2])
    route.append(nodes[3])
    route.update()

    cost_eval = CostEvaluator(1, 1, 0)
    op = ReverseSegment(ok_small)

    # Current (relevant) part of the route has distance:
    #   dist(4, 1) + dist(1, 2) + dist(2, 3) + dist(3, 0)
    #     = 1594 + 1992 + 621 + 2063
    #     = 6270.
    #
    # Proposed distance is:
    #   dist(4, 3) + dist(3, 2) + dist(2, 1) + dist(1, 0)
    #     = 828 + 647 + 1975 + 1726
    #     = 5176.
    #
    # Load remains the same, but the time warp decreases substantially as well:
    # from 3633 (due to visiting client 3 far too late) to 0. This results in
    # a total delta cost of -3633 + 5176 - 6270 = -4727.
    assert_allclose(op.evaluate(nodes[4], nodes[3], cost_eval), -4_727)

    # Check that applying the proposed move indeed creates the correct route.
    op.apply(nodes[4], nodes[3])
    assert_equal(route[1].client, 4)
    assert_equal(route[2].client, 3)
    assert_equal(route[3].client, 2)
    assert_equal(route[4].client, 1)


@pytest.mark.parametrize(
    "seed", [2643, 2742, 2941, 3457, 4299, 4497, 6178, 6434]
)
def test_RC208_instance(rc208, seed: int):
    """
    Test a larger instance over several seeds. We create a single route
    solution to ensure the operator is effective.
    """
    cost_evaluator = CostEvaluator(20, 6, 0)
    rng = RandomNumberGenerator(seed=seed)

    nb_params = NeighbourhoodParams(nb_granular=rc208.num_clients)
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208, nb_params))
    ls.add_node_operator(ReverseSegment(rc208))

    single_route = list(range(rc208.num_depots, rc208.num_locations))
    sol = Solution(rc208, [single_route])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)

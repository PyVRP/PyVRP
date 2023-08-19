from typing import List

import numpy as np
from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import (
    CostEvaluator,
    RandomNumberGenerator,
    Solution,
    VehicleType,
)
from pyvrp import (
    Route as SolRoute,
)
from pyvrp.search import (
    LocalSearch,
    NeighbourhoodParams,
    TwoOpt,
    compute_neighbours,
)
from pyvrp.search._search import Node, Route
from pyvrp.tests.helpers import make_heterogeneous, read


def test_OkSmall_instance():
    """
    Test 2-OPT on the OkSmall instance where we know exactly what's going on.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(TwoOpt(data))

    sol = Solution(data, [[1, 2, 3, 4]])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    assert_equal(improved_sol.num_routes(), 2)
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)

    # First improving (U, V) node pair is (1, 3), which results in the route
    # [1, 3, 2, 4]. The second improving node pair involves the depot of an
    # empty route: (1, 0). This results in routes [1] and [3, 2, 4].
    expected = Solution(data, [[1], [3, 2, 4]])
    assert_equal(improved_sol, expected)


@mark.parametrize(
    "vehicle_types",
    [
        [VehicleType(10, 1), VehicleType(10, 1)],
        [VehicleType(8, 1), VehicleType(10, 1)],
        [VehicleType(10, 1), VehicleType(8, 1)],
        [VehicleType(9, 1), VehicleType(9, 1)],
        [VehicleType(8, 1), VehicleType(8, 1)],
    ],
)
def test_OkSmall_multiple_vehicle_types(vehicle_types: List[VehicleType]):
    """
    This test evaluates a 2-OPT move that is improving for some of the vehicle
    types, and not for others. Because the granular neighbourhood is very
    constrained, only a single 2-OPT can be applied. When it's better to do so,
    it should have been applied, and when it's not better, it should not.
    """
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, vehicle_types)

    cost_evaluator = CostEvaluator(10_000, 6)  # large capacity penalty
    rng = RandomNumberGenerator(seed=42)

    neighbours: List[List[int]] = [[], [2], [], [], []]  # only 1 -> 2
    ls = LocalSearch(data, rng, neighbours)
    ls.add_node_operator(TwoOpt(data))

    routes1 = [SolRoute(data, [1, 3], 0), SolRoute(data, [2, 4], 1)]
    sol1 = Solution(data, routes1)

    routes2 = [SolRoute(data, [1, 4], 0), SolRoute(data, [2, 3], 1)]
    sol2 = Solution(data, routes2)

    cost1 = cost_evaluator.penalised_cost(sol1)
    cost2 = cost_evaluator.penalised_cost(sol2)
    assert_(not np.allclose(cost1, cost2))

    # Using the local search, the result should not get worse.
    improved_sol1 = ls.search(sol1, cost_evaluator)
    improved_sol2 = ls.search(sol2, cost_evaluator)

    expected_sol = sol1 if cost1 < cost2 else sol2
    assert_equal(improved_sol1, expected_sol)
    assert_equal(improved_sol2, expected_sol)


@mark.parametrize("seed", [2643, 2742, 2941, 3457, 4299, 4497, 6178, 6434])
def test_RC208_instance(seed: int):
    """
    Test a larger instance over several seeds.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=seed)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(TwoOpt(data))

    single_route = list(range(1, data.num_clients + 1))
    sol = Solution(data, [single_route])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)


def test_within_route_move():
    """
    Within-route 2-OPT reverses the segment between U and V. This test checks
    that such a move is correctly evaluated and applied on a single-route
    solution to the OkSmall instance.
    """
    data = read("data/OkSmall.txt")
    nodes = [Node(loc=loc) for loc in range(data.num_clients + 1)]

    # Current route is 4 -> 1 -> 2 -> 3.
    route = Route(data, idx=0, vehicle_type=0)
    route.append(nodes[4])
    route.append(nodes[1])
    route.append(nodes[2])
    route.append(nodes[3])
    route.update()

    cost_eval = CostEvaluator(1, 1)
    two_opt = TwoOpt(data)

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
    assert_equal(two_opt.evaluate(nodes[4], nodes[3], cost_eval), -4_727)

    # Check that applying the proposed move indeed creates the correct route.
    two_opt.apply(nodes[4], nodes[3])
    assert_equal(route[1].client, 4)
    assert_equal(route[2].client, 3)
    assert_equal(route[3].client, 2)
    assert_equal(route[4].client, 1)

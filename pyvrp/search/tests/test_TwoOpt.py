from typing import List

import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal
from pytest import mark

from pyvrp import (
    Client,
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
    VehicleType,
)
from pyvrp import Route as SolRoute
from pyvrp.search import (
    LocalSearch,
    NeighbourhoodParams,
    TwoOpt,
    compute_neighbours,
)
from pyvrp.search._search import Node, Route
from pyvrp.tests.helpers import make_heterogeneous


def test_OkSmall_instance(ok_small):
    """
    Test 2-OPT on the OkSmall instance where we know exactly what's going on.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_node_operator(TwoOpt(ok_small))

    sol = Solution(ok_small, [[1, 2, 3, 4]])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    assert_equal(improved_sol.num_routes(), 2)
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)

    # First improving (U, V) node pair is (1, 3), which results in the route
    # [1, 3, 2, 4]. The second improving node pair involves the depot of an
    # empty route: (1, 0). This results in routes [1] and [3, 2, 4].
    expected = Solution(ok_small, [[1], [3, 2, 4]])
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
def test_OkSmall_multiple_vehicle_types(
    ok_small, vehicle_types: List[VehicleType]
):
    """
    This test evaluates a 2-OPT move that is improving for some of the vehicle
    types, and not for others. Because the granular neighbourhood is very
    constrained, only a single 2-OPT can be applied. When it's better to do so,
    it should have been applied, and when it's not better, it should not.
    """
    data = make_heterogeneous(ok_small, vehicle_types)

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
def test_RC208_instance(rc208, seed: int):
    """
    Test a larger instance over several seeds.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=seed)

    nb_params = NeighbourhoodParams(nb_granular=rc208.num_clients)
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208, nb_params))
    ls.add_node_operator(TwoOpt(rc208))

    single_route = list(range(1, rc208.num_clients + 1))
    sol = Solution(rc208, [single_route])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)


def test_within_route_move(ok_small):
    """
    Within-route 2-OPT reverses the segment between U and V. This test checks
    that such a move is correctly evaluated and applied on a single-route
    solution to the OkSmall instance.
    """
    nodes = [Node(loc=loc) for loc in range(ok_small.num_clients + 1)]

    # Current route is 4 -> 1 -> 2 -> 3.
    route = Route(ok_small, idx=0, vehicle_type=0)
    route.append(nodes[4])
    route.append(nodes[1])
    route.append(nodes[2])
    route.append(nodes[3])
    route.update()

    cost_eval = CostEvaluator(1, 1)
    two_opt = TwoOpt(ok_small)

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
    assert_allclose(two_opt.evaluate(nodes[4], nodes[3], cost_eval), -4_727)

    # Check that applying the proposed move indeed creates the correct route.
    two_opt.apply(nodes[4], nodes[3])
    assert_equal(route[1].client, 4)
    assert_equal(route[2].client, 3)
    assert_equal(route[3].client, 2)
    assert_equal(route[4].client, 1)


def test_move_involving_empty_routes():
    """
    This test checks that a 2-OPT move that turns an empty route into a
    non-empty route (or vice-versa) is correctly evaluated.
    """
    data = ProblemData(
        clients=[Client(x=0, y=0), Client(x=1, y=1), Client(x=1, y=0)],
        vehicle_types=[VehicleType(0, 1, 10), VehicleType(0, 1, 100)],
        distance_matrix=np.zeros((3, 3), dtype=int),
        duration_matrix=np.zeros((3, 3), dtype=int),
    )

    route1 = Route(data, idx=0, vehicle_type=0)
    route2 = Route(data, idx=1, vehicle_type=1)

    for loc in [1, 2]:
        route1.append(Node(loc=loc))

    route1.update()  # depot -> 1 -> 2 -> depot
    route2.update()  # depot -> depot

    op = TwoOpt(data)
    cost_eval = CostEvaluator(0, 0)

    # This move does not change the route structure, so the delta cost is 0.
    assert_allclose(op.evaluate(route1[2], route2[0], cost_eval), 0)

    # This move creates routes (depot -> 1 -> depot) and (depot -> 2 -> depot),
    # making route 2 non-empty and thus incurring its fixed cost of 100.
    assert_allclose(op.evaluate(route1[1], route2[0], cost_eval), 100)

    # This move creates routes (depot -> depot) and (depot -> 1 -> 2 -> depot),
    # making route 1 empty, while making route 2 non-empty. The total fixed
    # cost incurred is thus -10 + 100 = 90.
    assert_allclose(op.evaluate(route1[0], route2[0], cost_eval), 90)

    # Now we reverse the visits of route 1 and 2, so that we can hit the cases
    # where route 1 is empty.
    route1.clear()

    for loc in [1, 2]:
        route2.append(Node(loc=loc))

    route1.update()  # depot -> depot
    route2.update()  # depot -> 1 -> 2 -> depot

    # This move does not change the route structure, so the delta cost is 0.
    assert_allclose(op.evaluate(route1[0], route2[2], cost_eval), 0)

    # This move creates routes (depot -> 2 -> depot) and (depot -> 1 -> depot),
    # making route 1 non-empty and thus incurring its fixed cost of 10.
    assert_allclose(op.evaluate(route1[0], route2[1], cost_eval), 10)

    # This move creates routes (depot -> 1 -> 2 -> depot) and (depot -> depot),
    # making route 1 non-empty, while making route 2 empty. The total fixed
    # cost incurred is thus 10 - 100 = -90.
    assert_allclose(op.evaluate(route1[0], route2[0], cost_eval), -90)

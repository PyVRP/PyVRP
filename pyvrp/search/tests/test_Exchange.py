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
    Exchange10,
    Exchange11,
    Exchange20,
    Exchange21,
    Exchange22,
    Exchange30,
    Exchange31,
    Exchange32,
    Exchange33,
    LocalSearch,
    NeighbourhoodParams,
    compute_neighbours,
)
from pyvrp.search._search import Node, Route
from pyvrp.tests.helpers import make_heterogeneous


@mark.parametrize(
    "operator",
    [
        Exchange11,
        Exchange21,
        Exchange22,
        Exchange31,
        Exchange32,
        Exchange33,
    ],
)
def test_swap_single_route_stays_single_route(rc208, operator):
    """
    Swap operators ((N, M)-exchange operators with M > 0) on a single route can
    only move within the same route, so they can never find a solution that has
    more than one route.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=rc208.num_clients)
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208, nb_params))
    ls.add_node_operator(operator(rc208))

    single_route = list(range(1, rc208.num_clients + 1))
    sol = Solution(rc208, [single_route])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    assert_equal(improved_sol.num_routes(), 1)
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)


@mark.parametrize("operator", [Exchange10, Exchange20, Exchange30])
def test_relocate_uses_empty_routes(rc208, operator):
    """
    Unlike the swapping exchange operators, relocate should be able to relocate
    clients to empty routes if that is an improvement.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=rc208.num_clients)
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208, nb_params))
    ls.add_node_operator(operator(rc208))

    single_route = list(range(1, rc208.num_clients + 1))
    sol = Solution(rc208, [single_route])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution, and
    # should use more routes.
    assert_(improved_sol.num_routes() > 1)
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)


@mark.parametrize(
    "operator",
    [
        Exchange22,
        Exchange30,
        Exchange31,
        Exchange32,
        Exchange33,
    ],
)
def test_cannot_exchange_when_parts_overlap_with_depot(ok_small, operator):
    """
    (N, M)-exchange works by exchanging N nodes starting at some node U with
    M nodes starting at some node V. But when there is no sequence of N or M
    nodes that does not contain the depot (because the routes are very short),
    then no exchange is possible.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_node_operator(operator(ok_small))

    sol = Solution(ok_small, [[1, 2], [3], [4]])
    new_sol = ls.search(sol, cost_evaluator)

    assert_equal(new_sol, sol)


@mark.parametrize("operator", [Exchange32, Exchange33])
def test_cannot_exchange_when_segments_overlap(ok_small, operator):
    """
    (3, 2)- and (3, 3)-exchange cannot exchange anything on a length-four
    single route solution: there's always overlap between the segments.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_node_operator(operator(ok_small))

    sol = Solution(ok_small, [[1, 2, 3, 4]])
    new_sol = ls.search(sol, cost_evaluator)

    assert_equal(new_sol, sol)


def test_cannot_swap_adjacent_segments(ok_small):
    """
    (2, 2)-exchange on a single route cannot swap adjacent segments, since
    that's already covered by (2, 0)-exchange.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_node_operator(Exchange22(ok_small))

    # An adjacent swap by (2, 2)-exchange could have created the single-route
    # solution [3, 4, 1, 2], which has a much lower cost. But that's not
    # allowed because adjacent swaps are not allowed.
    sol = Solution(ok_small, [[1, 2, 3, 4]])
    new_sol = ls.search(sol, cost_evaluator)

    assert_equal(new_sol, sol)


def test_swap_between_routes_OkSmall(ok_small):
    """
    On the OkSmall example, (2, 1)-exchange should be able to swap parts of a
    two route solution, resulting in something better.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_node_operator(Exchange21(ok_small))

    sol = Solution(ok_small, [[1, 2], [3, 4]])
    improved_sol = ls.search(sol, cost_evaluator)
    expected = Solution(ok_small, [[3, 4, 2], [1]])
    assert_equal(improved_sol, expected)

    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)


def test_relocate_after_depot_should_work(ok_small):
    """
    This test exercises the bug identified in issue #142, involving a relocate
    action that should insert directly after the depot.
    """
    op = Exchange10(ok_small)

    # We create two routes: one with clients [1, 2, 3], and the other empty.
    # It is an improving move to insert client 3 into the empty route.
    route1 = Route(ok_small, idx=0, vehicle_type=0)
    route2 = Route(ok_small, idx=1, vehicle_type=0)

    nodes = [Node(loc=client) for client in [1, 2, 3]]
    for node in nodes:
        route1.append(node)
    route1.update()

    # This solution can be improved by moving 3 into its own route, that is,
    # inserting it after the depot of an empty route. Before the bug was fixed,
    # (1, 0)-exchange never performed this move.
    cost_evaluator = CostEvaluator(20, 6)
    assert_(route1[3] is nodes[-1])
    assert_(op.evaluate(nodes[-1], route2[0], cost_evaluator) < 0)

    assert_(nodes[-1].route is route1)
    assert_equal(len(route1), 3)
    assert_equal(len(route2), 0)

    # Apply the move and check that the routes and nodes are appropriately
    # updated.
    op.apply(nodes[-1], route2[0])
    assert_(nodes[-1].route is route2)
    assert_equal(len(route1), 2)
    assert_equal(len(route2), 1)


def test_relocate_only_happens_when_distance_and_duration_allow_it():
    """
    Tests that (1, 0)-exchange checks the duration matrix for time-window
    feasibility before applying a move that improves the travelled distance.
    """
    clients = [
        Client(x=0, y=0, tw_early=0, tw_late=10),
        Client(x=1, y=0, tw_early=0, tw_late=5),
        Client(x=2, y=0, tw_early=0, tw_late=5),
    ]

    # Distance-wise, the best route is 0 -> 1 -> 2 -> 0. Duration-wise,
    # however, the best route is 0 -> 2 -> 1 -> 0.
    data = ProblemData(
        clients=clients,
        vehicle_types=[VehicleType(0, 1)],
        distance_matrix=np.asarray(
            [
                [0, 1, 5],
                [5, 0, 1],
                [1, 5, 0],
            ]
        ),
        duration_matrix=np.asarray(
            [
                [0, 100, 2],
                [1, 0, 100],
                [100, 2, 0],
            ]
        ),
    )

    # We consider two solutions. The first is duration optimal, and overall the
    # only feasible solution. This solution can thus not be improved further.
    # We also consider a distance-optimal solution that is not feasible. Since
    # we have non-zero time warp penalty, this solution should be improved into
    # the duration optimal solution.
    duration_optimal = Solution(data, [[2, 1]])
    distance_optimal = Solution(data, [[1, 2]])

    assert_(distance_optimal.distance() < duration_optimal.distance())
    assert_(duration_optimal.time_warp() < distance_optimal.time_warp())

    cost_evaluator = CostEvaluator(1, 1)
    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data))

    assert_equal(ls.search(duration_optimal, cost_evaluator), duration_optimal)
    assert_equal(ls.search(distance_optimal, cost_evaluator), duration_optimal)


def test_relocate_to_heterogeneous_empty_route(ok_small):
    """
    This test asserts that a customer will be relocated to a non-empty route
    with a different capacity even if there is another empty route in between.
    """
    vehicle_types = [VehicleType(cap, 1) for cap in [12, 5, 1, 3]]
    data = make_heterogeneous(ok_small, vehicle_types)

    # Use a huge cost for load penalties to make other aspects irrelevant
    cost_evaluator = CostEvaluator(100_000, 6)
    rng = RandomNumberGenerator(seed=42)

    # This is a non-empty neighbourhood (so LS does not complain), but the only
    # client moves allowed by it will not improve the initial solution created
    # below. So the only improvements (1, 0)-exchange can make must come from
    # moving clients behind the depot of a route.
    neighbours = [[] for _ in range(data.num_clients + 1)]
    neighbours[2].append(1)

    ls = LocalSearch(data, rng, neighbours)
    ls.add_node_operator(Exchange10(data))

    # The initial solution has routes with loads [13, 5, 0, 0]
    # with excess [1, 0, 0, 0]. Moving node 3 to route 4 will resolve all
    # load penalties, but other moves would increase load penalties.
    # Therefore, this requires moving to an empty route which is not the first.
    sol = Solution(
        data, [SolRoute(data, [1, 2, 3], 0), SolRoute(data, [4], 1)]
    )
    expected = Solution(
        data,
        [
            SolRoute(data, [1, 2], 0),
            SolRoute(data, [4], 1),
            SolRoute(data, [3], 3),
        ],
    )
    assert_equal(ls.search(sol, cost_evaluator), expected)


@mark.parametrize(
    ("op", "base_cost", "fixed_cost"),
    [
        (Exchange10, 2_346, 0),
        (Exchange10, 2_346, 100),
        (Exchange20, 1_417, 0),
        (Exchange20, 1_417, 9),
        (Exchange30, 135, 53),
        (Exchange30, 135, 997),
    ],
)
def test_relocate_fixed_vehicle_cost(ok_small, op, base_cost, fixed_cost):
    """
    Tests that relocate operators - (N, M)-exchange where M == 0 - also take
    into account fixed vehicle costs changes if one of the routes is empty. In
    particular, we fix the base cost of evaluating the route changes (that's
    not changed), and vary the fixed vehicle cost. The total delta cost should
    also vary as a result.
    """
    data = make_heterogeneous(ok_small, [VehicleType(10, 2, fixed_cost)])
    op = op(data)

    route1 = Route(data, idx=0, vehicle_type=0)
    for loc in [2, 4, 1, 3]:
        route1.append(Node(loc=loc))
    route1.update()

    route2 = Route(data, idx=1, vehicle_type=0)
    route2.update()

    # First route is not empty, second route is. The operator evaluates moving
    # some nodes to the second route, which would use both of them. That should
    # add to the fixed vehicle cost.
    cost_eval = CostEvaluator(1, 1)
    assert_allclose(
        op.evaluate(route1[1], route2[0], cost_eval), base_cost + fixed_cost
    )

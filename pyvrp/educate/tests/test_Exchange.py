from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import Client, CostEvaluator, Individual, ProblemData, XorShift128
from pyvrp.educate import LocalSearch, NeighbourhoodParams, compute_neighbours
from pyvrp.educate._Exchange import (
    Exchange10,
    Exchange11,
    Exchange20,
    Exchange21,
    Exchange22,
    Exchange30,
    Exchange31,
    Exchange32,
    Exchange33,
)
from pyvrp.tests.helpers import read


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
def test_swap_single_route_stays_single_route(operator):
    """
    Swap operators ((N, M)-exchange operators with M > 0) on a single route can
    only move within the same route, so they can never find a solution that has
    more than one route.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(operator(data))

    single_route = list(range(1, data.num_clients + 1))
    individual = Individual(data, [single_route])
    improved_individual = ls.search(individual, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    assert_equal(improved_individual.num_routes(), 1)
    current_cost = cost_evaluator.penalised_cost(individual)
    improved_cost = cost_evaluator.penalised_cost(improved_individual)
    assert_(improved_cost < current_cost)


@mark.parametrize(
    "operator",
    [
        Exchange10,
        Exchange20,
        Exchange30,
    ],
)
def test_relocate_uses_empty_routes(operator):
    """
    Unlike the swapping exchange operators, relocate should be able to relocate
    clients to empty routes if that is an improvement.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(operator(data))

    single_route = list(range(1, data.num_clients + 1))
    individual = Individual(data, [single_route])
    improved_individual = ls.search(individual, cost_evaluator)

    # The new solution should strictly improve on our original solution, and
    # should use more routes.
    assert_(improved_individual.num_routes() > 1)
    current_cost = cost_evaluator.penalised_cost(individual)
    improved_cost = cost_evaluator.penalised_cost(improved_individual)
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
def test_cannot_exchange_when_parts_overlap_with_depot(operator):
    """
    (N, M)-exchange works by exchanging N nodes starting at some node U with
    M nodes starting at some node V. But when there is no sequence of N or M
    nodes that does not contain the depot (because the routes are very short),
    then no exchange is possible.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(operator(data))

    individual = Individual(data, [[1, 2], [3], [4]])
    new_individual = ls.search(individual, cost_evaluator)

    assert_equal(new_individual, individual)


@mark.parametrize("operator", [Exchange32, Exchange33])
def test_cannot_exchange_when_segments_overlap(operator):
    """
    (3, 2)- and (3, 3)-exchange cannot exchange anything on a length-four
    single route solution: there's always overlap between the segments.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(operator(data))

    individual = Individual(data, [[1, 2, 3, 4]])
    new_individual = ls.search(individual, cost_evaluator)

    assert_equal(new_individual, individual)


def test_cannot_swap_adjacent_segments():
    """
    (2, 2)-exchange on a single route cannot swap adjacent segments, since
    that's already covered by (2, 0)-exchange.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(Exchange22(data))

    # An adjacent swap by (2, 2)-exchange could have created the single-route
    # solution [3, 4, 1, 2], which has a much lower cost. But that's not
    # allowed because adjacent swaps are not allowed.
    individual = Individual(data, [[1, 2, 3, 4]])
    new_individual = ls.search(individual, cost_evaluator)

    assert_equal(new_individual, individual)


def test_swap_between_routes_OkSmall():
    """
    On the OkSmall example, (2, 1)-exchange should be able to swap parts of a
    two route solution, resulting in something better.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(Exchange21(data))

    individual = Individual(data, [[1, 2], [3, 4]])
    improved_individual = ls.search(individual, cost_evaluator)
    expected = Individual(data, [[3, 4, 2], [1]])
    assert_equal(improved_individual, expected)

    current_cost = cost_evaluator.penalised_cost(individual)
    improved_cost = cost_evaluator.penalised_cost(improved_individual)
    assert_(improved_cost < current_cost)


def test_relocate_after_depot_should_work():
    """
    This test exercises the bug identified in issue #142, involving a relocate
    action that should insert directly after the depot.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    # This is a non-empty neighbourhood (so LS does not complain), but the only
    # client moves allowed by it will not improve the initial solution created
    # below. So the only improvements (1, 0)-exchange can make must come from
    # moving clients behind the depot of a route.
    neighbours = [[] for _ in range(data.num_clients + 1)]
    neighbours[2].append(1)

    ls = LocalSearch(data, rng, neighbours)
    ls.add_node_operator(Exchange10(data))

    # This individual can be improved by moving 3 into its own route, that is,
    # inserting it after the depot of an empty route. Before the bug was fixed,
    # (1, 0)-exchange never performed this move.
    individual = Individual(data, [[1, 2, 3], [4]])
    expected = Individual(data, [[1, 2], [3], [4]])
    assert_equal(ls.search(individual, cost_evaluator), expected)


def test_relocate_only_happens_when_distance_and_duration_allow_it():
    """
    Tests that (1, 0)-exchange checks the duration matrix for time-window
    feasibility before applying a move that improves the traveled distance.
    """
    clients = [
        Client(x=0, y=0, demand=0, service_duration=0, tw_early=0, tw_late=10),
        Client(x=1, y=0, demand=0, service_duration=0, tw_early=0, tw_late=5),
        Client(x=2, y=0, demand=0, service_duration=0, tw_early=0, tw_late=5),
    ]

    data = ProblemData(
        clients=clients,
        nb_vehicles=1,
        vehicle_cap=0,
        distance_matrix=[  # distance-wise, the best route is 0 -> 1 -> 2 -> 0.
            [0, 1, 5],
            [5, 0, 1],
            [1, 5, 0],
        ],
        duration_matrix=[  # duration-wise, the best route is 0 -> 2 -> 1 -> 0.
            [0, 100, 2],
            [1, 0, 100],
            [100, 2, 0],
        ],
    )

    # We consider two solutions. The first is duration optimal, and overall the
    # only feasible solution. This solution can thus not be improved further.
    # We also consider a distance-optimal solution that is not feasible. Since
    # we have non-zero time warp penalty, this solution should be improved into
    # the duration optimal solution.
    duration_optimal = Individual(data, [[2, 1]])
    distance_optimal = Individual(data, [[1, 2]])

    assert_(distance_optimal.distance() < duration_optimal.distance())
    assert_(duration_optimal.time_warp() < distance_optimal.time_warp())

    cost_evaluator = CostEvaluator(1, 1)
    rng = XorShift128(seed=42)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data))

    assert_equal(ls.search(duration_optimal, cost_evaluator), duration_optimal)
    assert_equal(ls.search(distance_optimal, cost_evaluator), duration_optimal)

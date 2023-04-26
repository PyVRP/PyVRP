from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import CostEvaluator, Individual, XorShift128
from pyvrp.educate import (
    LocalSearch,
    NeighbourhoodParams,
    Neighbours,
    compute_neighbours,
)
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

    assert_equal(improved_individual.get_routes(), [[3, 4, 2], [1], []])
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


def test_reoptimize_changed_objective_timewarp_OkSmall():
    """
    This test reproduces a bug where loadIndividual in LocalSearch.cpp would
    reset the timewarp for a route to 0 if the route was not changed. This
    would cause improving moves with a smaller timewarp not to be considered
    because the current cost doesn't count the current time warp.
    """
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=42)

    individual = Individual(data, [[1, 2, 3, 4]])

    # We make neighbours only contain 1 -> 2, so the only feasible move
    # is changing [1, 2, 3, 4] into [2, 1, 3, 4] or moving one of the nodes
    # into its own route. Since those solutions have larger distance but
    # smaller time warp, they are considered improving moves with a
    # sufficiently large time warp penalty.
    neighbours: Neighbours = [[], [2], [], [], []]  # 1 -> 2 only
    ls = LocalSearch(data, rng, neighbours)
    ls.add_node_operator(Exchange10(data))

    # With 0 timewarp penalty, the individual should not change since
    # the solution [2, 1, 3, 4] has larger distance
    improved_indiv = ls.search(individual, CostEvaluator(0, 0))
    assert_equal(individual, improved_indiv)

    # Now doing it again with a large TW penalty, we must find the alternative
    # solution
    # (previously this was not the case since due to caching the current TW was
    # computed as being zero, causing the move to be evaluated as worse)
    cost_evaluator_tw = CostEvaluator(0, 1000)
    improved_indiv = ls.search(individual, cost_evaluator_tw)
    # expected_indiv = Individual(data, [[2, 1, 3, 4]])
    # assert_equal(improved_indiv, expected_indiv)
    improved_cost = cost_evaluator_tw.penalised_cost(improved_indiv)
    assert improved_cost < cost_evaluator_tw.penalised_cost(individual)

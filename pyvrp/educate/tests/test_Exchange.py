from copy import deepcopy

from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import Individual, PenaltyManager, PenaltyParams, XorShift128
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
from pyvrp.tests.helpers import read, read_solution


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
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, nb_params))

    op = operator(data, pm)
    ls.add_node_operator(op)

    single_route = list(range(1, data.num_clients + 1))
    individual = Individual(data, pm, [single_route])
    copy = deepcopy(individual)

    ls.search(individual)

    # The new solution should strictly improve on our original solution.
    assert_equal(individual.num_routes(), 1)
    assert_(individual.cost() < copy.cost())


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
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, nb_params))

    op = operator(data, pm)
    ls.add_node_operator(op)

    single_route = list(range(1, data.num_clients + 1))
    individual = Individual(data, pm, [single_route])
    copy = deepcopy(individual)

    ls.search(individual)

    # The new solution should strictly improve on our original solution, and
    # should use more routes.
    assert_(individual.num_routes() > 1)
    assert_(individual.cost() < copy.cost())


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
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, nb_params))

    op = operator(data, pm)
    ls.add_node_operator(op)

    individual = Individual(data, pm, [[1, 2], [3], [4]])
    copy = deepcopy(individual)

    ls.search(individual)

    assert_equal(individual, copy)


@mark.parametrize("operator", [Exchange32, Exchange33])
def test_cannot_exchange_when_segments_overlap(operator):
    """
    (3, 2)- and (3, 3)-exchange cannot exchange anything on a length-four
    single route solution: there's always overlap between the segments.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, nb_params))

    op = operator(data, pm)
    ls.add_node_operator(op)

    individual = Individual(data, pm, [[1, 2, 3, 4]])
    copy = deepcopy(individual)

    ls.search(individual)

    assert_equal(individual, copy)


def test_cannot_swap_adjacent_segments():
    """
    (2, 2)-exchange on a single route cannot swap adjacent segments, since
    that's already covered by (2, 0)-exchange.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, nb_params))

    op = Exchange22(data, pm)
    ls.add_node_operator(op)

    # An adjacent swap by (2, 2)-exchange could have created the single-route
    # solution [3, 4, 1, 2], which has a much lower cost. But that's not
    # allowed because adjacent swaps are not allowed.
    individual = Individual(data, pm, [[1, 2, 3, 4]])
    copy = deepcopy(individual)

    ls.search(individual)

    assert_equal(individual, copy)


def test_swap_between_routes_OkSmall():
    """
    On the OkSmall example, (2, 1)-exchange should be able to swap parts of a
    two route solution, resulting in something better.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, nb_params))

    op = Exchange21(data, pm)
    ls.add_node_operator(op)

    individual = Individual(data, pm, [[1, 2], [3, 4]])
    copy = deepcopy(individual)

    ls.search(individual)

    assert_equal(individual.get_routes(), [[3, 4, 2], [1], []])
    assert_(individual.cost() < copy.cost())


def test_relocate_single_after_depot_RC2_10_8():
    """
    Route #8: [depot] 325 533 866 ...   -> cost 337934
    Route #8: [depot] 533 325 866   -> cost 337877

    When u = 533, v = 325 calling Exchange<1,0> should result in the
    second option since v->prev == depot, so we will try to move 533
    after the depot which will result in an improvement.
    """
    data = read(
        "../educate/tests/data/RC2_10_8.txt",
        instance_format="solomon",
        round_func="trunc1",
    )

    sol_before = read_solution(
        "../educate/tests/data/RC2_10_8_before_move_single_depot_533_325.sol"
    )
    sol_after = read_solution(
        "../educate/tests/data/RC2_10_8_after_move_single_depot_533_325.sol"
    )

    pen_params = PenaltyParams(
        init_capacity_penalty=157,
        init_time_warp_penalty=1,
        repair_booster=1,
    )
    pm = PenaltyManager(data.vehicle_capacity, params=pen_params)
    rng = XorShift128(seed=42)

    indiv_expected = Individual(data, pm, sol_after)

    # First test normal relocate (325 after 533)
    neighbours = [[] for _ in range(data.num_clients + 1)]
    neighbours[325].append(533)
    ls = LocalSearch(data, pm, rng, neighbours)

    op = Exchange10(data, pm)
    ls.add_node_operator(op)

    indiv = Individual(data, pm, sol_before)
    assert_(indiv_expected.cost() < indiv.cost())
    ls.search(indiv)
    assert_equal(indiv, indiv_expected)

    # Now test depot relocate (533 after prev(325) = depot)
    # First test normal relocate (325 after 533)
    neighbours = [[] for _ in range(data.num_clients + 1)]
    neighbours[533].append(325)
    ls = LocalSearch(data, pm, rng, neighbours)

    op = Exchange10(data, pm)
    ls.add_node_operator(op)

    indiv = Individual(data, pm, sol_before)
    assert_(indiv_expected.cost() < indiv.cost())
    ls.search(indiv)
    assert_equal(indiv, indiv_expected)

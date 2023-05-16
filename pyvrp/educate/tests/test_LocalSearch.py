from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import CostEvaluator, Individual, XorShift128
from pyvrp.educate import (
    Exchange10,
    Exchange11,
    LocalSearch,
    NeighbourhoodParams,
    Neighbours,
    compute_neighbours,
)
from pyvrp.tests.helpers import read


def test_local_search_raises_when_there_are_no_operators():
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    ls = LocalSearch(data, rng, compute_neighbours(data))
    individual = Individual.make_random(data, rng)

    with assert_raises(RuntimeError):
        ls.search(individual, cost_evaluator)

    with assert_raises(RuntimeError):
        ls.intensify(individual, cost_evaluator)


def test_local_search_raises_when_neighbourhood_structure_is_empty():
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=42)

    # Is completely empty neighbourhood, so there's nothing to do for the
    # local search in this case.
    neighbours = [[] for _ in range(data.num_clients + 1)]

    with assert_raises(RuntimeError):
        LocalSearch(data, rng, neighbours)

    ls = LocalSearch(data, rng, compute_neighbours(data))

    with assert_raises(RuntimeError):
        ls.set_neighbours(neighbours)


@mark.parametrize("size", [1, 2, 3, 4, 6, 7])  # num_clients + 1 == 5
def test_local_search_raises_when_neighbourhood_dimensions_do_not_match(size):
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=42)

    # Each of the given sizes is either smaller than or bigger than desired.
    neighbours = [[] for _ in range(size)]

    with assert_raises(RuntimeError):
        LocalSearch(data, rng, neighbours)

    ls = LocalSearch(data, rng, compute_neighbours(data))

    with assert_raises(RuntimeError):
        ls.set_neighbours(neighbours)


def test_local_search_raises_when_neighbourhood_contains_self_or_depot():
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=42)

    neighbours = [[client] for client in range(data.num_clients + 1)]

    with assert_raises(RuntimeError):
        LocalSearch(data, rng, neighbours)


@mark.parametrize(
    "weight_wait_time,"
    "weight_time_warp,"
    "nb_granular,"
    "symmetric_proximity,"
    "symmetric_neighbours",
    [
        (20, 20, 10, True, False),
        (20, 20, 10, True, True),
        # From original c++ implementation
        # (18, 20, 34, False),
        (18, 20, 34, True, True),
    ],
)
def test_local_search_set_get_neighbours(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    symmetric_proximity: bool,
    symmetric_neighbours: bool,
):
    data = read("data/RC208.txt", "solomon", round_func="trunc")

    seed = 42
    rng = XorShift128(seed=seed)

    params = NeighbourhoodParams(nb_granular=1)
    prev_neighbours = compute_neighbours(data, params)
    ls = LocalSearch(data, rng, prev_neighbours)

    params = NeighbourhoodParams(
        weight_wait_time,
        weight_time_warp,
        nb_granular,
        symmetric_proximity,
        symmetric_neighbours,
    )
    neighbours = compute_neighbours(data, params)

    # Test that before we set neighbours we don't have same
    assert_(ls.get_neighbours() != neighbours)

    # Test after we set we have the same
    ls.set_neighbours(neighbours)
    ls_neighbours = ls.get_neighbours()
    assert_equal(ls_neighbours, neighbours)

    # Check that the bindings make a copy (in both directions)
    assert_(ls_neighbours is not neighbours)
    ls_neighbours[1] = []
    assert_(ls.get_neighbours() != ls_neighbours)
    assert_equal(ls.get_neighbours(), neighbours)
    neighbours[1] = []
    assert_(ls.get_neighbours() != neighbours)


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
    improved_cost = cost_evaluator_tw.penalised_cost(improved_indiv)
    assert_(improved_cost < cost_evaluator_tw.penalised_cost(individual))


def test_prize_collecting():
    """
    Tests that local search works on a small prize-collecting instance.
    """
    data = read("data/p06-2-50.vrp", round_func="dimacs")
    rng = XorShift128(seed=42)
    cost_evaluator = CostEvaluator(1, 1)

    individual = Individual.make_random(data, rng)
    individual_cost = cost_evaluator.penalised_cost(individual)

    # Random solutions are complete...
    assert_equal(individual.num_clients(), data.num_clients)

    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data))  # relocate
    ls.add_node_operator(Exchange11(data))  # swap

    improved = ls.search(individual, cost_evaluator)
    improved_cost = cost_evaluator.penalised_cost(improved)

    # ...but an optimised prize-collecting solution is likely not complete.
    assert_(improved.num_clients() < individual.num_clients())
    assert_(improved_cost < individual_cost)

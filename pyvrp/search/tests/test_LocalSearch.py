from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import CostEvaluator, Route, Solution, VehicleType, XorShift128
from pyvrp.search import (
    Exchange10,
    Exchange11,
    LocalSearch,
    NeighbourhoodParams,
    RelocateStar,
    compute_neighbours,
)
from pyvrp.search._search import LocalSearch as cpp_LocalSearch
from pyvrp.tests.helpers import make_heterogeneous, read


def test_local_search_returns_same_solution_when_there_are_no_operators():
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    ls = LocalSearch(data, rng, compute_neighbours(data))
    sol = Solution.make_random(data, rng)

    # No operators have been added, so these calls should be no-ops.
    assert_equal(ls.search(sol, cost_evaluator), sol)
    assert_equal(ls.intensify(sol, cost_evaluator), sol)


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
    (
        "weight_wait_time",
        "weight_time_warp",
        "nb_granular",
        "symmetric_proximity",
        "symmetric_neighbours",
    ),
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
    rng = XorShift128(seed=42)

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
    This test reproduces a bug where loadSolution in LocalSearch.cpp would
    reset the timewarp for a route to 0 if the route was not changed. This
    would cause improving moves with a smaller timewarp not to be considered
    because the current cost doesn't count the current time warp.
    """
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=42)

    sol = Solution(data, [[1, 2, 3, 4]])

    # We make neighbours only contain 1 -> 2, so the only feasible move
    # is changing [1, 2, 3, 4] into [2, 1, 3, 4] or moving one of the nodes
    # into its own route. Since those solutions have larger distance but
    # smaller time warp, they are considered improving moves with a
    # sufficiently large time warp penalty.
    neighbours = [[], [2], [], [], []]  # 1 -> 2 only
    ls = LocalSearch(data, rng, neighbours)
    ls.add_node_operator(Exchange10(data))

    # With 0 timewarp penalty, the solution should not change since
    # the solution [2, 1, 3, 4] has larger distance
    improved_sol = ls.search(sol, CostEvaluator(0, 0))
    assert_equal(sol, improved_sol)

    # Now doing it again with a large TW penalty, we must find the alternative
    # solution
    # (previously this was not the case since due to caching the current TW was
    # computed as being zero, causing the move to be evaluated as worse)
    cost_evaluator_tw = CostEvaluator(0, 1000)
    improved_sol = ls.search(sol, cost_evaluator_tw)
    improved_cost = cost_evaluator_tw.penalised_cost(improved_sol)
    assert_(improved_cost < cost_evaluator_tw.penalised_cost(sol))


def test_prize_collecting():
    """
    Tests that local search works on a small prize-collecting instance.
    """
    data = read("data/p06-2-50.vrp", round_func="dimacs")
    rng = XorShift128(seed=42)
    cost_evaluator = CostEvaluator(1, 1)

    sol = Solution.make_random(data, rng)
    sol_cost = cost_evaluator.penalised_cost(sol)

    # Random solutions are complete...
    assert_equal(sol.num_clients(), data.num_clients)

    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data))  # relocate
    ls.add_node_operator(Exchange11(data))  # swap

    improved = ls.search(sol, cost_evaluator)
    improved_cost = cost_evaluator.penalised_cost(improved)

    # ...but an optimised prize-collecting solution is likely not complete.
    assert_(improved.num_clients() < sol.num_clients())
    assert_(improved_cost < sol_cost)


def test_cpp_shuffle_results_in_different_solution():
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    rng = XorShift128(seed=42)

    ls = cpp_LocalSearch(data, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data))
    ls.add_node_operator(Exchange11(data))

    cost_evaluator = CostEvaluator(1, 1)
    sol = Solution.make_random(data, rng)

    # LocalSearch::search is deterministic, so two calls with the same base
    # solution should result in the same improved solution.
    improved1 = ls.search(sol, cost_evaluator)
    improved2 = ls.search(sol, cost_evaluator)
    assert_(improved1 == improved2)

    # But the shuffle method changes the order in which moves are evaluated,
    # which should result in a very different search trajectory.
    ls.shuffle(rng)
    improved3 = ls.search(sol, cost_evaluator)
    assert_(improved3 != improved1)


def test_route_vehicle_types_are_preserved_for_locally_optimal_solutions():
    # This test tests that we will preserve vehicle types
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    rng = XorShift128(seed=42)

    neighbours = compute_neighbours(data)
    ls = cpp_LocalSearch(data, neighbours)
    ls.add_node_operator(Exchange10(data))
    ls.add_node_operator(Exchange11(data))

    cost_evaluator = CostEvaluator(1, 1)
    sol = Solution.make_random(data, rng)

    # LocalSearch::search is deterministic, so two calls with the same base
    # solution should result in the same improved solution.
    improved = ls.search(sol, cost_evaluator)

    # Now make the instance heterogeneous and update the local search
    data = make_heterogeneous(
        data, [VehicleType(1000, 25), VehicleType(1000, 25)]
    )
    ls = cpp_LocalSearch(data, neighbours)
    ls.add_node_operator(Exchange10(data))
    ls.add_node_operator(Exchange11(data))

    # Update the improved (locally optimal) solution with vehicles of type 1
    routes = [Route(data, r.visits(), 1) for r in improved.get_routes()]
    improved = Solution(data, routes)

    # Doing the search should not find any further improvements thus not change
    # the solution, especially not change the vehicle types
    further_improved = ls.search(improved, cost_evaluator)
    assert_equal(further_improved, improved)


def test_bugfix_vehicle_type_offsets():
    """
    See https://github.com/PyVRP/PyVRP/pull/292 for details. This exercises a
    fix to a bug that would crash local search due to an incorrect internal
    mapping of vehicle types to route indices if the next vehicle type had
    more vehicles than the previous.
    """
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, [VehicleType(10, 1), VehicleType(10, 2)])

    ls = cpp_LocalSearch(data, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data))

    cost_evaluator = CostEvaluator(1, 1)

    current = Solution(data, [Route(data, [1, 3], 1), Route(data, [2, 4], 1)])
    current_cost = cost_evaluator.penalised_cost(current)

    improved = ls.search(current, cost_evaluator)
    improved_cost = cost_evaluator.penalised_cost(improved)

    assert_(improved_cost <= current_cost)


def test_intensify_overlap_tolerance():
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    rng = XorShift128(seed=42)

    neighbours = compute_neighbours(data)
    ls = LocalSearch(data, rng, neighbours)
    ls.add_route_operator(RelocateStar(data))

    cost_eval = CostEvaluator(1, 1)
    sol = Solution.make_random(data, rng)

    # Overlap tolerance is zero, so no routes should have overlap and thus
    # no intensification should take place.
    unchanged = ls.intensify(sol, cost_eval, overlap_tolerance=0)
    assert_equal(unchanged, sol)

    # But with full overlap tolerance, all routes should be checked. That
    # should lead to an improvement over the random solution.
    better = ls.intensify(sol, cost_eval, overlap_tolerance=1)
    assert_(better != sol)
    assert_(cost_eval.penalised_cost(better) < cost_eval.penalised_cost(sol))


@mark.parametrize("tol", [-1.0, -0.01, 1.01, 10.9, 1000])
def test_intensify_overlap_tolerance_raises_outside_unit_interval(tol):
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    rng = XorShift128(seed=42)

    neighbours = compute_neighbours(data)
    ls = LocalSearch(data, rng, neighbours)
    ls.add_route_operator(RelocateStar(data))

    cost_eval = CostEvaluator(1, 1)
    sol = Solution.make_random(data, rng)

    with assert_raises(RuntimeError):  # each tolerance value is outside [0, 1]
        ls.intensify(sol, cost_eval, overlap_tolerance=tol)

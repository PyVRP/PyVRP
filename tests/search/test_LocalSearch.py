import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import (
    Client,
    ClientGroup,
    CostEvaluator,
    Depot,
    ProblemData,
    RandomNumberGenerator,
    Route,
    Solution,
    Trip,
    VehicleType,
)
from pyvrp.search import (
    Exchange10,
    Exchange11,
    LocalSearch,
    PerturbationManager,
    PerturbationParams,
    RelocateWithDepot,
    RemoveAdjacentDepot,
    compute_neighbours,
)
from pyvrp.search._search import LocalSearch as cpp_LocalSearch


def test_local_search_returns_same_solution_with_empty_neighbourhood(ok_small):
    """
    Tests that calling the local search when it only has binary operators and
    an empty neighbourhood is a no-op: since the binary operators respect the
    neighbourhood definition, they cannot do anything with an empty
    neighbourhood.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    neighbours = [[] for _ in range(ok_small.num_locations)]
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_operator(Exchange10(ok_small))
    ls.add_operator(Exchange11(ok_small))

    # The search is completed after one iteration due to the empty
    # neighbourhood. This also prevents moves involving empty routes,
    # which are not explicitly forbidden by the empty neighbourhood.
    sol = Solution.make_random(ok_small, rng)
    assert_equal(ls(sol, cost_evaluator, exhaustive=True), sol)


def test_local_search_call_perturbs_solution(ok_small):
    """
    Tests that calling local search perturbs a solution.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 0)

    # The local search should perturb the solution even though no node
    # operators are added.
    perturbed = ls(sol, cost_eval)
    assert_(perturbed != sol)


def test_get_set_neighbours(ok_small):
    """
    Tests that getting and setting the local search's granular neighbourhood
    works as expected. For more details, see the tests for the SearchSpace in
    ``test_SearchSpace.py``, which handle validation.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[] for _ in range(ok_small.num_locations)]
    ls = LocalSearch(ok_small, rng, neighbours)
    assert_equal(ls.neighbours, neighbours)

    new_neighbours = compute_neighbours(ok_small)
    assert_(new_neighbours != neighbours)

    ls.neighbours = new_neighbours
    assert_equal(ls.neighbours, new_neighbours)


def test_reoptimize_changed_objective_timewarp_OkSmall(ok_small):
    """
    This test reproduces a bug where loadSolution in LocalSearch.cpp would
    reset the timewarp for a route to 0 if the route was not changed. This
    would cause improving moves with a smaller timewarp not to be considered
    because the current cost doesn't count the current time warp.
    """
    rng = RandomNumberGenerator(seed=42)
    sol = Solution(ok_small, [[1, 2, 3, 4]])

    # We make neighbours only contain 1 -> 2, so the only feasible move
    # is changing [1, 2, 3, 4] into [2, 1, 3, 4] or moving one of the nodes
    # into its own route. Since those solutions have larger distance but
    # smaller time warp, they are considered improving moves with a
    # sufficiently large time warp penalty.
    neighbours = [[], [2], [], [], []]  # 1 -> 2 only
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_operator(Exchange10(ok_small))

    # With 0 timewarp penalty, the solution should not change since
    # the solution [2, 1, 3, 4] has larger distance.
    improved_sol = ls(sol, CostEvaluator([0], 0, 0), exhaustive=True)
    assert_equal(sol, improved_sol)

    # Now doing it again with a large TW penalty, we must find the alternative
    # solution
    # (previously this was not the case since due to caching the current TW was
    # computed as being zero, causing the move to be evaluated as worse)
    cost_evaluator_tw = CostEvaluator([0], 1000, 0)
    improved_sol = ls(sol, cost_evaluator_tw, exhaustive=True)
    improved_cost = cost_evaluator_tw.penalised_cost(improved_sol)
    assert_(improved_cost < cost_evaluator_tw.penalised_cost(sol))


def test_prize_collecting(prize_collecting):
    """
    Tests that local search works on a small prize-collecting instance.
    """
    rng = RandomNumberGenerator(seed=42)
    cost_evaluator = CostEvaluator([1], 1, 0)

    sol = Solution.make_random(prize_collecting, rng)
    sol_cost = cost_evaluator.penalised_cost(sol)

    neighbours = compute_neighbours(prize_collecting)
    ls = LocalSearch(prize_collecting, rng, neighbours)
    ls.add_operator(Exchange10(prize_collecting))  # relocate
    ls.add_operator(Exchange11(prize_collecting))  # swap

    improved = ls(sol, cost_evaluator, exhaustive=True)
    improved_cost = cost_evaluator.penalised_cost(improved)

    assert_(improved.num_clients() < prize_collecting.num_clients)
    assert_(improved_cost < sol_cost)


def test_cpp_shuffle_results_in_different_solution(rc208):
    """
    Tests that calling shuffle changes the evaluation order, which can well
    result in different solutions generated from the same initial solution.
    """
    rng = RandomNumberGenerator(seed=42)

    ls = cpp_LocalSearch(rc208, compute_neighbours(rc208))
    ls.add_operator(Exchange10(rc208))
    ls.add_operator(Exchange11(rc208))

    cost_evaluator = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(rc208, rng)

    # LocalSearch is deterministic, so two calls with the same base
    # solution should result in the same improved solution.
    improved1 = ls(sol, cost_evaluator)
    improved2 = ls(sol, cost_evaluator)
    assert_(improved1 == improved2)

    # But the shuffle method changes the order in which moves are evaluated,
    # which should result in a very different search trajectory.
    ls.shuffle(rng)
    improved3 = ls(sol, cost_evaluator)
    assert_(improved3 != improved1)


def test_vehicle_types_are_preserved_for_locally_optimal_solutions(rc208):
    """
    Tests that a solution that is already locally optimal returns the same
    solution, particularly w.r.t. the underlying vehicles. This exercises an
    issue where loading the solution in the local search did not preserve the
    vehicle types.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(rc208)

    ls = cpp_LocalSearch(rc208, neighbours)
    ls.add_operator(Exchange10(rc208))
    ls.add_operator(Exchange11(rc208))

    cost_evaluator = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(rc208, rng)

    improved = ls(sol, cost_evaluator, exhaustive=True)

    # Now make the instance heterogeneous and update the local search.
    data = rc208.replace(
        vehicle_types=[
            VehicleType(25, capacity=[10_000]),
            VehicleType(25, capacity=[10_000]),
        ]
    )

    ls = cpp_LocalSearch(data, neighbours)
    ls.add_operator(Exchange10(data))
    ls.add_operator(Exchange11(data))

    # Update the improved (locally optimal) solution with vehicles of type 1.
    routes = [Route(data, r.visits(), 1) for r in improved.routes()]
    improved = Solution(data, routes)

    # This should not find any further improvements and thus not change the
    # solution.
    further_improved = ls(improved, cost_evaluator, exhaustive=True)
    assert_equal(further_improved, improved)


def test_bugfix_vehicle_type_offsets(ok_small):
    """
    See https://github.com/PyVRP/PyVRP/pull/292 for details. This exercises a
    fix to a bug that would crash local search due to an incorrect internal
    mapping of vehicle types to route indices if the next vehicle type had
    more vehicles than the previous.
    """
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(1, capacity=[10]),
            VehicleType(2, capacity=[10]),
        ]
    )

    ls = cpp_LocalSearch(data, compute_neighbours(data))
    ls.add_operator(Exchange10(data))

    cost_evaluator = CostEvaluator([1], 1, 0)

    current = Solution(data, [Route(data, [1, 3], 1), Route(data, [2, 4], 1)])
    current_cost = cost_evaluator.penalised_cost(current)

    improved = ls(current, cost_evaluator, exhaustive=True)
    improved_cost = cost_evaluator.penalised_cost(improved)

    assert_(improved_cost <= current_cost)


def test_no_op_results_in_same_solution(ok_small):
    """
    Tests that calling local search without first adding any operators is a
    no-op, and returns the same solution as the one that was given to it.
    """
    rng = RandomNumberGenerator(seed=42)

    # Empty local search does not actually search anything, so it should return
    # the exact same solution as what was passed in.
    ls = LocalSearch(
        ok_small,
        rng,
        compute_neighbours(ok_small),
        PerturbationManager(PerturbationParams(0, 0)),  # disable perturbation
    )

    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)

    assert_equal(ls(sol, cost_eval, exhaustive=False), sol)
    assert_equal(ls(sol, cost_eval, exhaustive=True), sol)


def test_local_search_completes_incomplete_solutions(ok_small_prizes):
    """
    Tests that the local search object improve solutions that are incomplete,
    and returns a completed solution. Passing an incomplete solution should
    return a completed solution after search.
    """
    rng = RandomNumberGenerator(seed=42)

    ls = LocalSearch(ok_small_prizes, rng, compute_neighbours(ok_small_prizes))
    ls.add_operator(Exchange10(ok_small_prizes))

    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution(ok_small_prizes, [[2], [3, 4]])
    assert_(not sol.is_complete())  # 1 is required but not visited

    new_sol = ls(sol, cost_eval, exhaustive=True)
    assert_(new_sol.is_complete())


def test_replacing_optional_client():
    """
    Tests that the local search evaluates moves where an optional client is
    replaced with another that is not currently in the solution.
    """
    mat = [
        [0, 0, 0],
        [0, 0, 2],
        [0, 2, 0],
    ]
    data = ProblemData(
        clients=[
            Client(0, 0, tw_early=0, tw_late=1, prize=1, required=False),
            Client(0, 0, tw_early=0, tw_late=1, prize=5, required=False),
        ],
        depots=[Depot(0, 0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[mat],
        duration_matrices=[mat],
    )

    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_operator(Exchange10(data))

    # We start with a solution containing just client 1.
    sol = Solution(data, [[1]])
    assert_equal(sol.prizes(), 1)
    assert_(sol.is_feasible())

    # A unit of time warp has a penalty of 5 units, so it's never worthwhile to
    # have both clients 1 and 2 in the solution. However, replacing client 1
    # with 2 yields a prize of 5, rather than 1, at no additional cost.
    cost_eval = CostEvaluator([], 5, 0)
    improved = ls(sol, cost_eval)
    assert_equal(improved.prizes(), 5)
    assert_(improved.is_feasible())


def test_mutually_exclusive_group(gtsp):
    """
    Smoke test that runs the local search on a medium-size TSP instance with
    fifty mutually exclusive client groups.
    """
    assert_equal(gtsp.num_groups, 50)

    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(gtsp)
    perturbation = PerturbationManager(PerturbationParams(0, 0))

    ls = LocalSearch(gtsp, rng, neighbours, perturbation)
    ls.add_operator(Exchange10(gtsp))

    sol = Solution.make_random(gtsp, rng)
    cost_eval = CostEvaluator([20], 6, 0)
    improved = ls(sol, cost_eval)

    assert_(not sol.is_group_feasible())
    assert_(improved.is_group_feasible())

    sol_cost = cost_eval.penalised_cost(sol)
    improved_cost = cost_eval.penalised_cost(improved)
    assert_(improved_cost < sol_cost)


def test_mutually_exclusive_group_not_in_solution(
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that the local search inserts a client from the mutually exclusive
    group if the entire group is missing from the solution.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small_mutually_exclusive_groups)

    ls = LocalSearch(ok_small_mutually_exclusive_groups, rng, neighbours)
    ls.add_operator(Exchange10(ok_small_mutually_exclusive_groups))

    sol = Solution(ok_small_mutually_exclusive_groups, [[4]])
    assert_(not sol.is_group_feasible())

    improved = ls(sol, CostEvaluator([20], 6, 0))
    assert_(improved.is_group_feasible())


def test_swap_if_improving_mutually_exclusive_group(
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that we swap a client (1) in a mutually exclusive group when another
    client (3) in the group is better to have.
    """
    data = ok_small_mutually_exclusive_groups
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(data)
    perturbation = PerturbationManager(PerturbationParams(0, 0))

    ls = LocalSearch(data, rng, neighbours, perturbation)
    ls.add_operator(Exchange10(data))

    cost_eval = CostEvaluator([20], 6, 0)
    sol = Solution(data, [[1, 4]])
    improved = ls(sol, cost_eval, exhaustive=True)
    assert_(cost_eval.penalised_cost(improved) < cost_eval.penalised_cost(sol))

    routes = improved.routes()
    assert_equal(improved.num_routes(), 1)
    assert_equal(routes[0].visits(), [3, 4])


def test_no_op_multi_trip_instance(ok_small_multiple_trips):
    """
    Tests that loading and exporting a multi-trip instance correctly returns an
    equivalent solution when no operators are available.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[] for _ in range(ok_small_multiple_trips.num_locations)]
    ls = LocalSearch(
        ok_small_multiple_trips,
        rng,
        neighbours,
        PerturbationManager(PerturbationParams(0, 0)),  # disable perturbation
    )

    trip1 = Trip(ok_small_multiple_trips, [1, 2], 0)
    trip2 = Trip(ok_small_multiple_trips, [3, 4], 0)
    route = Route(ok_small_multiple_trips, [trip1, trip2], 0)

    sol = Solution(ok_small_multiple_trips, [route])
    cost_eval = CostEvaluator([20], 6, 0)
    assert_equal(ls(sol, cost_eval), sol)


def test_local_search_inserts_reload_depots(ok_small_multiple_trips):
    """
    Tests that the local search routine inserts a reload depot when that is
    beneficial.
    """
    rng = RandomNumberGenerator(seed=2)
    neighbours = compute_neighbours(ok_small_multiple_trips)

    ls = LocalSearch(ok_small_multiple_trips, rng, neighbours)
    ls.add_operator(RelocateWithDepot(ok_small_multiple_trips))

    sol = Solution(ok_small_multiple_trips, [[1, 2, 3, 4]])
    assert_(sol.has_excess_load())

    cost_eval = CostEvaluator([1_000], 0, 0)
    improved = ls(sol, cost_eval)

    assert_(not improved.has_excess_load())
    assert_(cost_eval.penalised_cost(improved) < cost_eval.penalised_cost(sol))

    assert_equal(improved.num_routes(), 1)
    assert_equal(improved.num_trips(), 2)
    assert_(not improved.has_excess_load())


def test_local_search_removes_useless_reload_depots(ok_small_multiple_trips):
    """
    Tests that the local search removes useless reload depots from the given
    solution.
    """
    data = ok_small_multiple_trips
    rng = RandomNumberGenerator(seed=2)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_operator(Exchange10(data))

    route1 = Route(data, [Trip(data, [1], 0), Trip(data, [3], 0)], 0)
    route2 = Route(data, [2, 4], 0)
    sol = Solution(data, [route1, route2])

    cost_eval = CostEvaluator([1_000], 0, 0)
    improved = ls(sol, cost_eval)
    assert_(cost_eval.penalised_cost(improved) < cost_eval.penalised_cost(sol))

    # The local search should have removed the reload depot from the first
    # route, because that was not providing any value.
    routes = improved.routes()
    assert_(str(routes[0]), "1 3")
    assert_(str(routes[1]), "2 4")


def test_search_statistics(ok_small):
    """
    Tests that the local search's search statistics return meaningful
    information about the number of evaluated and improving moves.
    """
    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(
        ok_small,
        rng,
        compute_neighbours(ok_small),
        PerturbationManager(PerturbationParams(0, 0)),  # disable perturbation
    )

    op = Exchange10(ok_small)
    ls.add_operator(op)

    # No solution is yet loaded/improved, so all these numbers should be zero.
    stats = ls.statistics
    assert_equal(stats.num_moves, 0)
    assert_equal(stats.num_improving, 0)
    assert_equal(stats.num_updates, 0)

    # Load and improve a random solution. This should result in a non-zero
    # number of moves.
    rnd_sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 1)
    improved = ls(rnd_sol, cost_eval, exhaustive=True)

    stats = ls.statistics
    assert_(stats.num_moves > 0)
    assert_(stats.num_improving > 0)
    assert_(stats.num_updates >= stats.num_improving)

    # Since we have only a single operator, the number of moves and the number
    # of improving moves should match what the operator tracks.
    assert_equal(stats.num_moves, op.statistics.num_evaluations)
    assert_equal(stats.num_improving, op.statistics.num_applications)

    # The improved solution is already locally optimal, so it cannot be further
    # improved by the local search. The number of improving moves should thus
    # be zero after another attempt.
    ls(improved, cost_eval, exhaustive=True)

    stats = ls.statistics
    assert_(stats.num_moves > 0)
    assert_equal(stats.num_improving, 0)
    assert_equal(stats.num_updates, 0)


def test_operators_property(ok_small):
    """
    Tests adding and accessing operators to the LocalSearch object.
    """
    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small))

    # The local search has not yet been equipped with operators, so it should
    # start empty.
    assert_equal(len(ls.unary_operators), 0)
    assert_equal(len(ls.binary_operators), 0)

    # Now we add a binary operator. The local search does not take ownership,
    # so its only operator should be the exact object we just created.
    op = Exchange10(ok_small)
    ls.add_operator(op)
    assert_equal(len(ls.unary_operators), 0)
    assert_equal(len(ls.binary_operators), 1)
    assert_(ls.binary_operators[0] is op)

    # And similarly for a unary operator.
    op = RemoveAdjacentDepot(ok_small)
    ls.add_operator(op)
    assert_equal(len(ls.unary_operators), 1)
    assert_equal(len(ls.binary_operators), 1)
    assert_(ls.unary_operators[0] is op)


@pytest.mark.parametrize(
    ("instance", "exp_clients"),
    [
        # {1, 2, 3, 4} are all required clients.
        ("ok_small", {1, 2, 3, 4}),
        # 1 from required group {1, 2, 3}, 4 is a required client.
        ("ok_small_mutually_exclusive_groups", {3, 4}),
    ],
)
def test_inserts_required_missing(instance, exp_clients: set[int], request):
    """
    Tests that the local search inserts all missing clients and groups, if
    those are currently missing from the solution.
    """
    data = request.getfixturevalue(instance)
    rng = RandomNumberGenerator(seed=42)
    perturbation = PerturbationManager(PerturbationParams(1, 1))
    ls = LocalSearch(data, rng, compute_neighbours(data), perturbation)
    ls.add_operator(Exchange10(data))

    sol = Solution(data, [])
    assert_(not sol.is_complete())

    cost_eval = CostEvaluator([20], 6, 0)
    improved = ls(sol, cost_eval)
    assert_(improved.is_complete())

    visits = {client for route in improved.routes() for client in route}
    assert_equal(visits, exp_clients)


def test_local_search_exhaustive(rc208):
    """
    Tests calling the local search with the optional ``exhaustive`` argument
    for a complete evaluation.
    """
    rng = RandomNumberGenerator(seed=2)
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208))
    ls.add_operator(Exchange10(rc208))

    init = Solution.make_random(rc208, rng)
    cost_eval = CostEvaluator([20], 6, 0)

    # The returned solution by default evaluates only around perturbed,
    # promising clients. That is not a full search. But when exhaustive is
    # explicitly is explicitly set, a full search must be done. The resulting
    # solution should be better than what's returned after perturbation,
    # because a full search evaluates many more moves.
    perturbed = ls(init, cost_eval, exhaustive=False)
    exhaustive = ls(init, cost_eval, exhaustive=True)

    perturbed_cost = cost_eval.penalised_cost(perturbed)
    exhaustive_cost = cost_eval.penalised_cost(exhaustive)
    assert_(exhaustive_cost < perturbed_cost)

    # Both should also be better than the initial, random solution.
    init_cost = cost_eval.penalised_cost(init)
    assert_(perturbed_cost < init_cost)
    assert_(exhaustive_cost < init_cost)


def test_local_search_inserts_into_empty_solutions():
    """
    Tests that the local search inserts into empty solutions, even when the
    granular neighbourhood is empty.
    """
    data = ProblemData(
        clients=[
            Client(0, 0, prize=1_000, required=False),  # high prizes make
            Client(0, 0, prize=1_000, required=False),  # inserting worthwhile
        ],
        depots=[Depot(0, 0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    rng = RandomNumberGenerator(seed=2)
    cost_eval = CostEvaluator([], 0, 0)
    ls = LocalSearch(data, rng, [[], [], []])
    ls.add_operator(Exchange10(data))

    empty = Solution(data, [])
    assert_equal(empty.num_clients(), 0)
    assert_equal(empty.uncollected_prizes(), 2_000)

    # Start from the empty solution and check that the improved solution is no
    # longer empty - the local search should have inserted the missing clients.
    sol = ls(empty, cost_eval, exhaustive=True)
    assert_equal(sol.num_clients(), 2)
    assert_equal(sol.uncollected_prizes(), 0)


def test_does_not_insert_optional_groups():
    """
    Tests that the local search does not insert optional groups, and in fact
    completely removes those if that's more beneficial.
    """
    matrix = np.ones((3, 3), dtype=int)
    np.fill_diagonal(matrix, 0)

    # Instance with all optional clients, that are in turn part of an optional
    # group. The group isn't worth visiting since that incurs distance cost,
    # and there is no prize to be obtained.
    data = ProblemData(
        clients=[
            Client(x=0, y=0, group=0, required=False),
            Client(x=0, y=0, group=0, required=False),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
        groups=[ClientGroup([1, 2], required=False)],
    )

    rng = RandomNumberGenerator(seed=2)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_operator(Exchange10(data))

    # Start with a full solution. After local search, the solution should be
    # empty because the group is not worth keeping around.
    sol = Solution(data, [[1, 2]])
    cost_eval = CostEvaluator([], 0, 0)
    improved = ls(sol, cost_eval, exhaustive=True)
    assert_equal(improved.num_clients(), 0)

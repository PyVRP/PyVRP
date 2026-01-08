import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import (
    Client,
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
    SwapRoutes,
    SwapStar,
    compute_neighbours,
)
from pyvrp.search._search import LocalSearch as cpp_LocalSearch


def test_local_search_returns_same_solution_with_empty_neighbourhood(ok_small):
    """
    Tests that calling the local search when it only has node operators and
    an empty neighbourhood is a no-op: since the node operators respect the
    neighbourhood definition, they cannot do anything with an empty
    neighbourhood.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    neighbours = [[] for _ in range(ok_small.num_locations)]
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_node_operator(Exchange10(ok_small))
    ls.add_node_operator(Exchange11(ok_small))

    # The search is completed after one iteration due to the empty
    # neighbourhood. This also prevents moves involving empty routes,
    # which are not explicitly forbidden by the empty neighbourhood.
    sol = Solution.make_random(ok_small, rng)
    assert_equal(ls.search(sol, cost_evaluator), sol)


def test_local_search_call_perturbs_solution(ok_small):
    """
    Tests that calling local search perturbs a solution.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 0)

    # The local search should perturb the solution even though no node and
    # route operators are added.
    perturbed = ls(sol, cost_eval)
    assert_(perturbed != sol)


def test_local_search_call_exhaustive(ok_small):
    """
    Tests that calling local search with exhaustive=True does not perturb
    the solution.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 0)

    # With exhaustive=True and no operators, the solution should not be
    # changed since there is no perturbation and no local search operators.
    result = ls(sol, cost_eval, exhaustive=True)
    assert_equal(result, sol)


def test_local_search_exhaustive_still_improves_with_operators(ok_small):
    """
    Tests that exhaustive=True skips perturbation but still performs
    local search when operators are present.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_node_operator(Exchange10(ok_small))

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 0)
    initial_cost = cost_eval.penalised_cost(sol)

    # exhaustive=True: no perturbation, but search still runs
    result = ls(sol, cost_eval, exhaustive=True)
    result_cost = cost_eval.penalised_cost(result)

    # Should be improved (or equal) by local search operators
    assert_(result_cost <= initial_cost)


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
    ls.add_node_operator(Exchange10(ok_small))

    # With 0 timewarp penalty, the solution should not change since
    # the solution [2, 1, 3, 4] has larger distance.
    improved_sol = ls.search(sol, CostEvaluator([0], 0, 0))
    assert_equal(sol, improved_sol)

    # Now doing it again with a large TW penalty, we must find the alternative
    # solution
    # (previously this was not the case since due to caching the current TW was
    # computed as being zero, causing the move to be evaluated as worse)
    cost_evaluator_tw = CostEvaluator([0], 1000, 0)
    improved_sol = ls.search(sol, cost_evaluator_tw)
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
    ls.add_node_operator(Exchange10(prize_collecting))  # relocate
    ls.add_node_operator(Exchange11(prize_collecting))  # swap

    improved = ls.search(sol, cost_evaluator)
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
    ls.add_node_operator(Exchange10(rc208))
    ls.add_node_operator(Exchange11(rc208))

    cost_evaluator = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(rc208, rng)

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
    ls.add_node_operator(Exchange10(rc208))
    ls.add_node_operator(Exchange11(rc208))

    cost_evaluator = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(rc208, rng)

    improved = ls.search(sol, cost_evaluator)

    # Now make the instance heterogeneous and update the local search.
    data = rc208.replace(
        vehicle_types=[
            VehicleType(25, capacity=[10_000]),
            VehicleType(25, capacity=[10_000]),
        ]
    )

    ls = cpp_LocalSearch(data, neighbours)
    ls.add_node_operator(Exchange10(data))
    ls.add_node_operator(Exchange11(data))

    # Update the improved (locally optimal) solution with vehicles of type 1.
    routes = [Route(data, r.visits(), 1) for r in improved.routes()]
    improved = Solution(data, routes)

    # Doing the search should not find any further improvements thus not change
    # the solution.
    further_improved = ls.search(improved, cost_evaluator)
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
    ls.add_node_operator(Exchange10(data))

    cost_evaluator = CostEvaluator([1], 1, 0)

    current = Solution(data, [Route(data, [1, 3], 1), Route(data, [2, 4], 1)])
    current_cost = cost_evaluator.penalised_cost(current)

    improved = ls.search(current, cost_evaluator)
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

    assert_equal(ls(sol, cost_eval), sol)
    assert_equal(ls.search(sol, cost_eval), sol)
    assert_equal(ls.intensify(sol, cost_eval), sol)


def test_intensify_can_improve_solution_further(rc208):
    """
    Tests that ``intensify()`` improves a solution further once ``search()`` is
    stuck.
    """
    rng = RandomNumberGenerator(seed=11)

    ls = LocalSearch(rc208, rng, compute_neighbours(rc208))
    ls.add_node_operator(Exchange11(rc208))
    ls.add_route_operator(SwapStar(rc208))

    cost_eval = CostEvaluator([1], 1, 0)

    # The following solution is locally optimal w.r.t. the node operators. This
    # solution cannot be improved further by repeated calls to ``search()``.
    search_opt = ls.search(Solution.make_random(rc208, rng), cost_eval)
    search_cost = cost_eval.penalised_cost(search_opt)

    # But it can be improved further using the intensifying route operators,
    # as the following solution shows.
    intensify_opt = ls.intensify(search_opt, cost_eval)
    intensify_cost = cost_eval.penalised_cost(intensify_opt)

    assert_(intensify_cost < search_cost)

    # Both solutions are locally optimal. ``search_opt`` w.r.t. to the node
    # operators, and ``intensify_opt`` w.r.t. to the route operators. Repeated
    # calls to ``search()`` and ``intensify`` do not result in further
    # improvements for such locally optimal solutions.
    for _ in range(10):
        assert_equal(ls.search(search_opt, cost_eval), search_opt)
        assert_equal(ls.intensify(intensify_opt, cost_eval), intensify_opt)


def test_intensify_can_swap_routes(ok_small):
    """
    Tests that the bug identified in #742 is fixed. The intensify method should
    be able to improve a solution by swapping routes.
    """
    rng = RandomNumberGenerator(seed=42)

    data = ok_small.replace(
        vehicle_types=[
            VehicleType(1, capacity=[5]),
            VehicleType(1, capacity=[20]),
        ]
    )
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_route_operator(SwapRoutes(data))

    # High load penalty, so the solution is penalised for having excess load.
    cost_eval = CostEvaluator([100_000], 0, 0)
    route1 = Route(data, [1, 2, 3], 0)  # Excess load: 13 - 5 = 8
    route2 = Route(data, [4], 1)  # Excess load: 0
    init_sol = Solution(data, [route1, route2])
    init_cost = cost_eval.penalised_cost(init_sol)

    assert_equal(init_sol.excess_load(), [8])

    # This solution can be improved by using the intensifying route operators
    # to swap the routes in the solution.
    intensify_sol = ls.intensify(init_sol, cost_eval)
    intensify_cost = cost_eval.penalised_cost(intensify_sol)

    assert_(intensify_cost < init_cost)
    assert_equal(intensify_sol.excess_load(), [0])


def test_local_search_completes_incomplete_solutions(ok_small_prizes):
    """
    Tests that the local search object improve solutions that are incomplete,
    and returns a completed solution. Passing an incomplete solution should
    return a completed solution after search.
    """
    rng = RandomNumberGenerator(seed=42)

    ls = LocalSearch(ok_small_prizes, rng, compute_neighbours(ok_small_prizes))
    ls.add_node_operator(Exchange10(ok_small_prizes))

    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution(ok_small_prizes, [[2], [3, 4]])
    assert_(not sol.is_complete())  # 1 is required but not visited

    new_sol = ls.search(sol, cost_eval)
    assert_(new_sol.is_complete())


def test_local_search_does_not_remove_required_clients():
    """
    Tests that the local search object does not remove required clients, even
    when that might result in a significant cost improvement.
    """
    rng = RandomNumberGenerator(seed=42)
    data = ProblemData(
        clients=[
            # This client cannot be removed, even though it causes significant
            # load violations.
            Client(x=1, y=1, delivery=[100], required=True),
            # This client can and should be removed, because the prize is not
            # worth the detour.
            Client(x=2, y=2, delivery=[0], prize=0, required=False),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(1, capacity=[50])],
        distance_matrices=[np.where(np.eye(3), 0, 10)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data))

    sol = Solution(data, [[1, 2]])
    assert_(sol.is_complete())

    # Test that the improved solution contains the first client, but removes
    # the second. The first client is required, so could not be removed, but
    # the second could and that is an improving move.
    cost_eval = CostEvaluator([100], 100, 0)
    new_sol = ls.search(sol, cost_eval)
    assert_equal(new_sol.num_clients(), 1)
    assert_(new_sol.is_complete())

    sol_cost = cost_eval.penalised_cost(sol)
    new_cost = cost_eval.penalised_cost(new_sol)
    assert_(new_cost < sol_cost)


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
    ls.add_node_operator(Exchange10(data))

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
    ls.add_node_operator(Exchange10(gtsp))

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
    ls.add_node_operator(Exchange10(ok_small_mutually_exclusive_groups))

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
    ls.add_node_operator(Exchange10(data))

    cost_eval = CostEvaluator([20], 6, 0)
    sol = Solution(data, [[1, 4]])
    improved = ls(sol, cost_eval)
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
    ls.add_node_operator(RelocateWithDepot(ok_small_multiple_trips))

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
    ls.add_node_operator(Exchange10(data))

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

    node_op = Exchange10(ok_small)
    ls.add_node_operator(node_op)

    # No solution is yet loaded/improved, so all these numbers should be zero.
    stats = ls.statistics
    assert_equal(stats.num_moves, 0)
    assert_equal(stats.num_improving, 0)
    assert_equal(stats.num_updates, 0)

    # Load and improve a random solution. This should result in a non-zero
    # number of moves.
    rnd_sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 1)
    improved = ls(rnd_sol, cost_eval)

    stats = ls.statistics
    assert_(stats.num_moves > 0)
    assert_(stats.num_improving > 0)
    assert_(stats.num_updates >= stats.num_improving)

    # Since we have only a single node operator, the number of moves and the
    # number of improving moves should match what the node operator tracks.
    assert_equal(stats.num_moves, node_op.statistics.num_evaluations)
    assert_equal(stats.num_improving, node_op.statistics.num_applications)

    # The improved solution is already locally optimal, so it cannot be further
    # improved by the local search. The number of improving moves should thus
    # be zero after another attempt.
    ls(improved, cost_eval)

    stats = ls.statistics
    assert_(stats.num_moves > 0)
    assert_equal(stats.num_improving, 0)
    assert_equal(stats.num_updates, 0)


def test_node_and_route_operators_property(ok_small):
    """
    Tests adding and accessing node and route operators to the LocalSearch
    object.
    """
    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small))

    # The local search has not yet been equipped with operators, so it should
    # start empty.
    assert_equal(len(ls.node_operators), 0)
    assert_equal(len(ls.route_operators), 0)

    # Now we add a node operator. The local search does not take ownership, so
    # its only node operator should be the exact same object as the one we just
    # created.
    node_op = Exchange10(ok_small)
    ls.add_node_operator(node_op)
    assert_equal(len(ls.node_operators), 1)
    assert_(ls.node_operators[0] is node_op)

    # And a route operator, for which the same should hold.
    route_op = SwapStar(ok_small)
    ls.add_route_operator(route_op)
    assert_equal(len(ls.route_operators), 1)
    assert_(ls.route_operators[0] is route_op)


@pytest.mark.parametrize(
    ("instance", "exp_clients"),
    [
        # {1, 2, 3, 4} are all required clients.
        ("ok_small", {1, 2, 3, 4}),
        # 1 from required group {1, 2, 3}, 4 is a required client.
        ("ok_small_mutually_exclusive_groups", {1, 4}),
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
    ls.add_node_operator(Exchange10(data))

    sol = Solution(data, [])
    assert_(not sol.is_complete())

    cost_eval = CostEvaluator([20], 6, 0)
    improved = ls(sol, cost_eval)
    assert_(improved.is_complete())

    visits = {client for route in improved.routes() for client in route}
    assert_equal(visits, exp_clients)

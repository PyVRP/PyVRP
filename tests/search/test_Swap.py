import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import (
    Client,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    RandomNumberGenerator,
    Solution,
    VehicleType,
)
from pyvrp.search import (
    LocalSearch,
    NeighbourhoodParams,
    Swap11,
    Swap21,
    Swap22,
    Swap31,
    Swap32,
    Swap33,
    compute_neighbours,
)
from pyvrp.search._search import Node
from tests.helpers import make_search_route


@pytest.mark.parametrize(
    "operator",
    [Swap11, Swap21, Swap22, Swap31, Swap32, Swap33],
)
def test_swap_single_route_stays_single_route(rc208, operator):
    """
    Swap operators on a single route can only move within the same route, so
    they can never find a solution that has more than one route.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(num_neighbours=rc208.num_clients)
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208, nb_params))
    ls.add_operator(operator(rc208))

    single_route = list(range(rc208.num_clients))
    sol = Solution(rc208, [single_route])
    improved_sol = ls(sol, cost_evaluator, exhaustive=True)

    # The new solution should strictly improve on our original solution.
    assert_equal(improved_sol.num_routes(), 1)
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)


@pytest.mark.parametrize("operator", [Swap22, Swap31, Swap32, Swap33])
def test_cannot_swap_when_parts_overlap_with_depot(ok_small, operator):
    """
    SwapNM works by swapping N nodes starting at some node U with M nodes
    starting at some node V. But when there is no sequence of N or M nodes that
    does not contain the depot (because the routes are very short), then no
    swap is possible.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(num_neighbours=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_operator(operator(ok_small))

    sol = Solution(ok_small, [[0, 1], [2], [3]])
    new_sol = ls(sol, cost_evaluator, exhaustive=True)

    assert_equal(new_sol, sol)


@pytest.mark.parametrize("operator", [Swap32, Swap33])
def test_cannot_swap_when_segments_overlap(ok_small, operator):
    """
    Swap32 and Swap33 cannot swap anything on a length-four single route
    solution: there's always overlap between the segments.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(num_neighbours=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_operator(operator(ok_small))

    sol = Solution(ok_small, [[0, 1, 2, 3]])
    new_sol = ls(sol, cost_evaluator, exhaustive=True)

    assert_equal(new_sol, sol)


def test_cannot_swap_adjacent_segments(ok_small):
    """
    Swap22 on a single route cannot swap adjacent segments, since that's
    already covered by Relocate2.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(num_neighbours=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_operator(Swap22(ok_small))

    # An adjacent swap by Swap22 could have created the single-route solution
    # [C2, C3, C0, C1], which has a much lower cost. But that's not allowed
    # because adjacent swaps are not allowed.
    sol = Solution(ok_small, [[0, 1, 2, 3]])
    new_sol = ls(sol, cost_evaluator, exhaustive=True)

    assert_equal(new_sol, sol)


def test_swap_between_routes_OkSmall(ok_small):
    """
    On the OkSmall example, Swap21 should be able to swap parts of a two route
    solution, resulting in something better.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(num_neighbours=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_operator(Swap21(ok_small))

    sol = Solution(ok_small, [[0, 1], [2, 3]])
    improved_sol = ls(sol, cost_evaluator, exhaustive=True)
    expected = Solution(ok_small, [[2, 3, 1], [0]])
    assert_equal(improved_sol, expected)

    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)


@pytest.mark.parametrize(("max_dur", "cost"), [(0, -693), (5_000, -596)])
def test_swap_with_duration_constraint(ok_small, max_dur, cost):
    """
    Tests that the swap operators correctly evaluate time warp due to maximum
    shift duration violations.
    """
    vehicle_type = VehicleType(2, capacity=[10], shift_duration=max_dur)
    data = ok_small.replace(vehicle_types=[vehicle_type])
    op = Swap21(data)

    route1 = make_search_route(data, ["C1", "C3"])
    route2 = make_search_route(data, ["C0", "C2"])

    # Without duration constraint, route1 has a duration of 5_229 and no time
    # warp while route2 has a duration of 5_814 and timewarp 2_087, for a net
    # duration of 5_814 - 2_087 = 3_727 so no violation.
    # Consolidation into a single route may or may not be improving as the
    # total distance decreases but the maximum duration violation increases.
    # Moving from the first to the second route reduces the maximum duration
    # violation in the first route and is typically improving.
    assert_equal(route1.duration(), 5_229)
    assert_equal(route2.duration(), 5_814)

    cost_eval = CostEvaluator([1], 1, 0)
    delta, should_apply = op.evaluate(route1[1], route2[1], cost_eval)
    assert_equal(delta, cost)
    assert_equal(should_apply, cost < 0)


def test_swap_within_route_simultaneous_pickup_and_delivery():
    """
    Tests that the swap operator correctly evaluates load violations within
    the same route.
    """
    data = ProblemData(
        locations=[
            Location(0, 0),
            Location(1, 0),
            Location(2, 0),
            Location(2, 0),
        ],
        clients=[
            Client(location=1, pickup=[5]),
            Client(location=2, pickup=[0]),
            Client(location=3, delivery=[5]),
        ],
        depots=[Depot(location=0)],
        vehicle_types=[VehicleType(capacity=[5])],
        distance_matrices=[np.where(np.eye(4), 0, 1)],
        duration_matrices=[np.zeros((4, 4), dtype=int)],
    )

    # Route is C0 -> C1 -> C2, and gets C0's pickup amount (5) before dropping
    # off C2's delivery amount (5). So total load is 10, and the excess load 5.
    route = make_search_route(data, ["C0", "C1", "C2"])
    assert_(not route.is_feasible())
    assert_equal(route.load(), [10])
    assert_equal(route.excess_load(), [5])

    # We evaluate swapping C0 and C2, which would resolve the excess load: the
    # important bit is that we visit C2 before C0.
    op = Swap11(data)
    cost_eval = CostEvaluator([1], 1, 0)
    assert_equal(op.evaluate(route[1], route[3], cost_eval), (-5, True))


@pytest.mark.parametrize(
    ("max_distance", "expected"),
    [
        (5_000, -5_222),
        (2_500, -6_072),  # both routes now violate max dist constraint
        (0, -6_072),  # so tighter constraints do not improve anything
    ],
)
def test_swap_max_distance(ok_small, max_distance: int, expected: int):
    """
    Tests that a swap move correctly evaluates maximum distance constraint
    violations, and can identify improving moves that increase overall distance
    but reduce the maximum distance violation.
    """
    vehicle_type = VehicleType(2, capacity=[10], max_distance=max_distance)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    route1 = make_search_route(data, ["C0", "C1"])
    route2 = make_search_route(data, ["C2"])

    assert_equal(route1.distance(), 5_501)
    assert_equal(route1.excess_distance(), max(5_501 - max_distance, 0))

    assert_equal(route2.distance(), 3_994)
    assert_equal(route2.excess_distance(), max(3_994 - max_distance, 0))

    cost_eval = CostEvaluator([0], 0, 10)
    op = Swap11(data)

    # Swapping client C1 in route1 and client C2 in route2 improves the overall
    # distance and reduces the excess distance violations.
    actual, should_apply = op.evaluate(route1[2], route2[1], cost_eval)
    assert_equal(actual, expected)
    assert_(should_apply)
    op.apply(route1[2], route2[1])

    route1.update()
    assert_equal(route1.distance(), 5_034)
    assert_equal(route1.excess_distance(), max(5_034 - max_distance, 0))

    route2.update()
    assert_equal(route2.distance(), 3_909)
    assert_equal(route2.excess_distance(), max(3_909 - max_distance, 0))

    delta_dist = 5_034 + 3_909 - 5_501 - 3_994  # compare manual delta cost
    delta_excess = sum(
        [
            max(5_034 - max_distance, 0),
            max(3_909 - max_distance, 0),
            -max(5_501 - max_distance, 0),
            -max(3_994 - max_distance, 0),
        ]
    )
    assert_equal(delta_dist + 10 * delta_excess, expected)


def test_swap_with_different_profiles(ok_small_two_profiles):
    """
    Tests that swap correctly evaluates moves between routes with different
    profiles.
    """
    data = ok_small_two_profiles

    route1 = make_search_route(data, ["C2"], vehicle_type=0)
    route2 = make_search_route(data, ["C3"], vehicle_type=1)

    op = Swap11(data)
    cost_eval = CostEvaluator([0], 0, 0)  # all zero so no costs from penalties

    # This move evaluates swapping C2 and C3 between routes. The cost delta
    # is as follows, taking into account the different profiles' distances.
    dist1, dist2 = data.distance_matrices()
    delta = dist1[0, 4] + dist1[4, 0] + dist2[0, 3] + dist2[3, 0]
    delta -= route1.distance() + route2.distance()
    assert_equal(op.evaluate(route1[1], route2[1], cost_eval), (delta, False))


def test_swap_does_not_swap_depots(ok_small_multiple_trips):
    """
    Tests that the swap operator does not attempt moves that include moving
    a reload depot.
    """
    data = ok_small_multiple_trips
    route = make_search_route(data, ["C0", "C1", "D0", "C2", "C3"])

    op = Swap21(data)
    cost_eval = CostEvaluator([0], 0, 0)

    # This move overlaps with reload depot at index 3, so cannot be evaluated.
    assert_equal(op.evaluate(route[2], route[4], cost_eval), (0, False))


def test_bug_evaluating_move_with_initial_load():
    """
    Tests a bug where previously the move evaluated below would claim to result
    in an improvement. See #813 for details.
    """
    data = ProblemData(
        locations=[
            Location(0, 0),
            Location(0, 0),
            Location(0, 0),
            Location(0, 0),
        ],
        clients=[
            Client(location=1, delivery=[1]),
            Client(location=2, delivery=[1]),
            Client(location=3, delivery=[0]),
        ],
        depots=[Depot(location=0)],
        vehicle_types=[VehicleType(2, capacity=[5], initial_load=[5])],
        distance_matrices=[np.zeros((4, 4), dtype=int)],
        duration_matrices=[np.zeros((4, 4), dtype=int)],
    )

    op = Swap21(data)
    cost_eval = CostEvaluator([1], 0, 0)

    route1 = make_search_route(data, ["C1", "C2"])
    route2 = make_search_route(data, ["C0"])

    # This move just permutes the solution, turning route1 into route2, and
    # vice versa. Thus, the delta cost of this move should be zero.
    assert_equal(op.evaluate(route1[1], route2[1], cost_eval), (0, False))


@pytest.mark.parametrize("operator", [Swap21, Swap33])
@pytest.mark.parametrize("instance", ["ok_small", "pr107", "prize_collecting"])
def test_supports_clients(operator, instance, request):
    """
    Tests that the swap operators support any type of data instance with
    regular clients.
    """
    data = request.getfixturevalue(instance)
    assert_(operator.supports(data))


def test_supports_shipments(small_shipments):
    """
    Tests that only the even swap operators support instances with pure
    shipments.
    """
    # This is an instance with pure shipments - there are no clients.
    assert_equal(small_shipments.num_clients, 0)
    assert_equal(small_shipments.num_shipments, 4)

    # These move an even number of nodes between routes, and thus support
    # instances with pure shipments.
    assert_(Swap22.supports(small_shipments))

    # But these operators move an odd number of nodes between routes, and that
    # is not supported.
    assert_(not Swap11.supports(small_shipments))
    assert_(not Swap21.supports(small_shipments))


def test_skip_unassigned_clients(ok_small):
    """
    Tests that the operators do not evaluate moves for unassigned clients.
    """
    route = make_search_route(ok_small, ["C0", "C1"])
    node = Node("C2")  # unassigned

    operator = Swap11(ok_small)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(operator.evaluate(node, route[0], cost_eval), (0, False))


def test_name(ok_small):
    """
    Tests accessing the operator's name attribute.
    """
    assert_equal(Swap11(ok_small).name, "Swap11")
    assert_equal(Swap31(ok_small).name, "Swap31")


def test_swap_shipment(small_shipments):
    """
    Tests swapping two shipments.
    """
    activities = ["L1", "U1", "L0", "U0", "L2", "U2", "L3", "U3"]
    route = make_search_route(small_shipments, activities)
    assert_equal(route.distance(), 64_267)

    op = Swap22(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    # These cannot be swapped since they would move part of a shipment,
    # possibly resulting in a pickup after a delivery.
    assert_equal(op.evaluate(route[2], route[0], cost_eval), (0, False))
    assert_equal(op.evaluate(route[4], route[0], cost_eval), (0, False))
    assert_equal(op.evaluate(route[6], route[0], cost_eval), (0, False))

    # But swapping L0 U0 with L3 U3 is fine, and an improving move.
    assert_equal(op.evaluate(route[3], route[7], cost_eval), (-5_622, True))
    op.apply(route[3], route[7])
    route.update()

    assert_equal(route.distance(), 58_645)
    assert_equal(str(route), "L1 U1 L3 U3 L2 U2 L0 U0")

import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator, Depot, ProblemData, VehicleType
from pyvrp.search import RemoveOptional
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_does_not_remove_required_clients():
    """
    Tests that RemoveOptional does not remove required clients, even when that
    might result in a significant cost improvement.
    """
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

    route = make_search_route(data, [1, 2])
    cost_eval = CostEvaluator([100], 100, 0)
    op = RemoveOptional(data)

    # Test that the the operator cannot remove the first required client, but
    # does want to remove the second.
    assert_equal(op.evaluate(route[1], cost_eval), (0, False))
    assert_equal(op.evaluate(route[2], cost_eval), (-10, True))

    # Should remove the second client.
    op.apply(route[2])
    assert_equal(str(route), "1")


def test_supports(
    ok_small,
    ok_small_prizes,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that RemoveOptional supports prize-collecting instances, but not
    instances with only required clients.
    """
    assert_(not RemoveOptional.supports(ok_small))
    assert_(RemoveOptional.supports(ok_small_prizes))
    assert_(RemoveOptional.supports(ok_small_mutually_exclusive_groups))


def test_skip_only_in_required_group(ok_small_mutually_exclusive_groups):
    """
    Tests that RemoveOptional only removes clients in required groups when
    there is more than one group client in the solution.
    """
    data = ok_small_mutually_exclusive_groups
    solution = Solution(data)
    route = solution.routes[0]
    route.append(solution.nodes[1])
    route.append(solution.nodes[2])
    route.update()

    op = RemoveOptional(data)
    cost_eval = CostEvaluator([0], 0, 0)
    op.init(solution)

    # The mutually exclusive group is in the solution twice, with both clients
    # 1 and 2. That's infeasible, and removing either is an improving move.
    assert_equal(op.evaluate(route[1], cost_eval), (-1_592, True))
    assert_equal(op.evaluate(route[2], cost_eval), (-2_231, True))

    # Remove the second client. There's now only one client left in the
    # solution for this group.
    op.apply(route[2])
    route.update()

    # And that means we cannot remove the last client, since that would render
    # the solution group infeasible.
    assert_equal(op.evaluate(route[1], cost_eval), (0, False))


def test_fixed_vehicle_cost():
    """
    Tests that RemoveOptional subtracts the fixed vehicle cost if the route
    will become empty.
    """
    cost_eval = CostEvaluator([], 0, 0)
    data = ProblemData(
        clients=[
            Client(x=1, y=1, required=False),
            Client(x=1, y=0, required=False),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(fixed_cost=7), VehicleType(fixed_cost=13)],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    op = RemoveOptional(data)

    # All distances, durations, and loads are equal. So the only cost change
    # can happen due to vehicle changes. In this, case we evaluate removing the
    # only client on a route. That makes the route empty, and removes the fixed
    # vehicle cost of 7 for this vehicle type.
    route = make_search_route(data, [1], vehicle_type=0)
    assert_equal(op.evaluate(route[1], cost_eval), (-7, True))

    # Same story for this route, but now we have a different vehicle type with
    # fixed cost 13.
    route = make_search_route(data, [1], vehicle_type=1)
    assert_equal(op.evaluate(route[1], cost_eval), (-13, True))


def test_remove(ok_small_prizes):
    """
    Tests that RemoveOptional works correctly on a specific example.
    """
    cost_eval = CostEvaluator([1], 1, 0)
    route = make_search_route(ok_small_prizes, [1, 2])

    op = RemoveOptional(ok_small_prizes)

    # Purely distance. Removes arcs 1 -> 2 -> 0, adds arcs 1 -> 0. This change
    # has delta distance of 1726 - 1992 - 1965 = -2231, and a prize delta of
    # 15: -2231 + 15 = -2216.
    assert_equal(op.evaluate(route[2], cost_eval), (-2216, True))


def test_empty_route_delta_cost_bug():
    """
    Tests that a bug identified in #853 has been fixed. Before fixing this bug,
    when removing a client introduced an empty route, that incorrectly included
    the empty route's costs. In practice, we want empty routes to have zero
    cost, even if they violate constraints.
    """
    mat = [
        [0, 5, 1],
        [5, 0, 1],
        [1, 1, 0],
    ]
    data = ProblemData(
        depots=[Depot(x=0, y=0), Depot(x=0, y=0)],
        clients=[Client(x=0, y=0, required=False)],
        vehicle_types=[
            # Vehicle type has time warp because just travelling between depots
            # violates the shift_duration constraint.
            VehicleType(1, start_depot=0, end_depot=1, shift_duration=0),
        ],
        duration_matrices=[mat],
        distance_matrices=[mat],
    )

    op = RemoveOptional(data)
    cost_eval = CostEvaluator([], 1, 1)

    # Similarly, if removing a client results in an empty route, then we should
    # not include the empty route's costs.
    route = make_search_route(data, [2])
    assert_equal(op.evaluate(route[1], cost_eval), (-4, True))

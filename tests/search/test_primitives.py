import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator, Depot, ProblemData, VehicleType
from pyvrp.search._search import (
    Node,
    Route,
    inplace_cost,
    insert_cost,
    remove_cost,
)
from tests.helpers import make_search_route


def test_insert_cost_zero_when_not_allowed(ok_small):
    """
    Tests that insert_cost() returns zero when a move is not possible. This is
    the only case where it shortcuts to return a non-negative delta cost.
    """
    cost_eval = CostEvaluator([1], 1, 0)
    route = make_search_route(ok_small, [1, 2])

    # Inserting the depot is not possible.
    assert_equal(insert_cost(route[0], route[1], ok_small, cost_eval), 0)
    assert_equal(insert_cost(route[3], route[2], ok_small, cost_eval), 0)

    # Inserting after a node that's not in a route is not possible.
    assert_equal(insert_cost(route[1], Node(loc=3), ok_small, cost_eval), 0)


def test_insert_cost(ok_small):
    """
    Tests that the insert_cost() method works correctly on a few basic cases.
    """
    cost_eval = CostEvaluator([1], 1, 0)
    route = make_search_route(ok_small, [1, 2])

    # This adds arcs 1 -> 4 -> 2, and removes arcs 1 -> 2. The added distances
    # is 1593 + 1090, the removed distance 1992. This also adds 5 additional
    # delivery demand, which exceeds the vehicle capacity by 5, so we get an
    # additional load penalty of 5. There is no time warp. Total delta cost:
    #     1593 + 1090 - 1992 + 5 = 696.
    assert_equal(insert_cost(Node(loc=4), route[1], ok_small, cost_eval), 696)

    # +5 load penalty, and delta dist is 1090 + 1475 - 1965 = 600. So total
    # delta cost is 605 (again no time warp).
    assert_equal(insert_cost(Node(loc=4), route[2], ok_small, cost_eval), 605)

    # Now we do have some time warp changes. +3 load penalty, delta dist is
    # 1427 + 647 - 1992 = 82, but time warp increases because we cannot get to
    # client 3 quick enough from client 1. In particular, we can arrive at
    # client 1 at 15600, service for 360, and then travel to client 3 for
    # duration 1427. Then we arrive at client 3 at 17387, which is after its
    # closing time window of 15300. So we add 17387 - 15300 = 2087 time warp.
    # Tallying it all up, total delta cost:
    #      3 + 82 + 2087 = 2172.
    assert_equal(insert_cost(Node(loc=3), route[1], ok_small, cost_eval), 2172)

    # +3 load penalty, delta dist is 621 + 2063 - 1965 = 719. Time warp
    # increases because we have to visit clients 1 and 2 before we visit
    # client 3. We get to client 1 at 15600, service for 360, travel to client
    # 2 for 1992, service for 360, and then travel to client 3 for 621, where
    # we arrive at 18933, after its closing time window of 15300. This adds
    # 18933 - 15300 = 3633 time warp. Tallying it all up, total delta cost:
    #     3 + 719 + 3633 = 4355.
    assert_equal(insert_cost(Node(loc=3), route[2], ok_small, cost_eval), 4355)


def test_insert_cost_between_different_depots(ok_small_multi_depot):
    """
    Tests that insert_cost() correctly determines the delta distance cost of
    inserting a new node in an empty route with two different depots.
    """
    vehicle_type = VehicleType(3, [10], start_depot=0, end_depot=1)
    data = ok_small_multi_depot.replace(vehicle_types=[vehicle_type])

    route = Route(data, idx=0, vehicle_type=0)
    route.update()

    cost_eval = CostEvaluator([0], 0, 0)
    dist_mat = data.distance_matrix(0)

    # The delta cost is the distance from the start depot to the new node, and
    # back to the end depot. The route is currently empty so it does not
    # contribute to the delta cost.
    delta = dist_mat[0, 2] + dist_mat[2, 1]
    assert_equal(insert_cost(Node(loc=2), route[0], data, cost_eval), delta)


def test_remove_cost_zero_when_not_allowed(ok_small):
    """
    Tests that remove_cost() returns zero when a move is not possible. This is
    the only case where it shortcuts to return a non-negative delta cost.
    """
    cost_eval = CostEvaluator([1], 1, 0)
    route = make_search_route(ok_small, [1, 2])

    # Removing the depot is not possible.
    assert_equal(remove_cost(route[0], ok_small, cost_eval), 0)
    assert_equal(remove_cost(route[3], ok_small, cost_eval), 0)

    # Removing a node that's not in a route is not possible.
    assert_equal(remove_cost(Node(loc=3), ok_small, cost_eval), 0)


def test_remove(ok_small):
    """
    Tests that the remove_cost() method works correctly on a few basic cases.
    """
    cost_eval = CostEvaluator([1], 1, 0)
    route = make_search_route(ok_small, [1, 2])

    # Purely distance. Removes arcs 0 -> 1 -> 2, adds arc 0 -> 2. This change
    # has delta distance of 1944 - 1544 - 1992 = -1592.
    assert_equal(remove_cost(route[1], ok_small, cost_eval), -1592)

    # Purely distance. Removes arcs 1 -> 2 -> 0, adds arcs 1 -> 0. This change
    # has delta distance of 1726 - 1992 - 1965 = -2231.
    assert_equal(remove_cost(route[2], ok_small, cost_eval), -2231)


def test_insert_fixed_vehicle_cost():
    """
    Tests that insert_cost() adds the fixed vehicle cost if the route is empty.
    """
    cost_eval = CostEvaluator([], 0, 0)
    data = ProblemData(
        clients=[Client(x=1, y=1), Client(x=1, y=0)],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(fixed_cost=7), VehicleType(fixed_cost=13)],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    # All distances, durations, and loads are equal. So the only cost change
    # can happen due to vehicle changes. In this, case we evaluate inserting a
    # client into an empty route. That adds the fixed vehicle cost of 7 for
    # this vehicle type.
    route = Route(data, idx=0, vehicle_type=0)
    assert_equal(insert_cost(Node(loc=1), route[0], data, cost_eval), 7)

    # Same story for this route, but now we have a different vehicle type with
    # fixed cost 13.
    route = Route(data, idx=0, vehicle_type=1)
    assert_equal(insert_cost(Node(loc=1), route[0], data, cost_eval), 13)


def test_remove_fixed_vehicle_cost():
    """
    Tests that remove_cost() subtracts the fixed vehicle cost if the route will
    be left empty.
    """
    cost_eval = CostEvaluator([], 0, 0)
    data = ProblemData(
        clients=[Client(x=1, y=1), Client(x=1, y=0)],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(fixed_cost=7), VehicleType(fixed_cost=13)],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    # All distances, durations, and loads are equal. So the only cost change
    # can happen due to vehicle changes. In this, case we evaluate removing the
    # only client on a route. That makes the route empty, and removes the fixed
    # vehicle cost of 7 for this vehicle type.
    route = make_search_route(data, [1], vehicle_type=0)
    assert_equal(remove_cost(route[1], data, cost_eval), -7)

    # Same story for this route, but now we have a different vehicle type with
    # fixed cost 13.
    route = make_search_route(data, [1], vehicle_type=1)
    assert_equal(remove_cost(route[1], data, cost_eval), -13)


def test_inplace_cost_zero_when_shortcutting_on_guard_clauses(ok_small):
    """
    Tests that inplace_cost() returns zero cost when it hits one of the guard
    clauses: either when the first node is in a route, or the second node is
    not.
    """
    cost_eval = CostEvaluator([1], 1, 0)
    route = Route(ok_small, idx=0, vehicle_type=0)
    node1 = Node(loc=1)
    node2 = Node(loc=2)

    # node1 is in the route, so it cannot be inserted in place of node2, which
    # is not in a route. This should shortcut to return 0.
    route.append(node1)
    assert_(node1.route and not node2.route)
    assert_equal(inplace_cost(node1, node2, ok_small, cost_eval), 0)

    # Remove node1. Now neither node is in a route. That should also shortcut.
    route.clear()
    assert_(not node1.route and not node2.route)
    assert_equal(inplace_cost(node1, node2, ok_small, cost_eval), 0)

    # Now both nodes are in a route. So node1 cannot be inserted, since it is
    # already in a route. That should shortcut.
    route.append(node1)
    route.append(node2)
    assert_(node1.route and node2.route)
    assert_equal(inplace_cost(node1, node2, ok_small, cost_eval), 0)


def test_inplace_cost_delta_distance_computation(ok_small):
    """
    Tests that inplace_cost() correctly evaluates the delta distance of an
    improving move.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    node1 = Node(loc=1)
    node2 = Node(loc=2)
    node3 = Node(loc=3)

    route.append(node1)
    route.append(node2)
    route.update()

    # Evaluate the cost of replacing node1 by node3. This would be the route
    # 3 -> 2, rather than 1 -> 2. That saves costs dist(0, 1) + dist(1, 2), and
    # adds cost dist(0, 3) + dist(3, 2), for a total savings of:
    #   dist(0, 1) + dist(1, 2) = 3536
    #   dist(0, 3) + dist(3, 2) = 2578
    #                     delta = -958
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(inplace_cost(node3, node1, ok_small, cost_eval), -958)


def test_remove_reload_depot(ok_small_multiple_trips):
    """
    Tests that remove_cost() correctly evaluates removing a reload depot.
    """
    data = ok_small_multiple_trips
    route = make_search_route(data, [1, 2, 0, 3, 4])

    assert_(not route.has_excess_load())
    assert_(route[3].is_reload_depot())

    # If we remove the reload depot, we gain 8 excess load. That costs 8_000
    # with this cost evaluator. Additionally, we have some changes in distance
    # cost, as follows:
    #              dist(2, 3) = 621
    # dist(2, 0) + dist(0, 3) = 1965 + 1931
    #              dist delta = -3275
    cost_eval = CostEvaluator([1000], 0, 0)
    assert_equal(remove_cost(route[3], data, cost_eval), 8_000 - 3_275)


def test_remove_consecutive_reload_depots(ok_small_multiple_trips):
    """
    Tests that removing one of multiple, consecutive reload depots is evaluated
    correctly.
    """
    veh_type = ok_small_multiple_trips.vehicle_type(0).replace(max_reloads=2)
    data = ok_small_multiple_trips.replace(vehicle_types=[veh_type])
    route = make_search_route(data, [1, 2, 0, 0, 3, 4])

    assert_(route[3].is_reload_depot())
    assert_(route[4].is_reload_depot())

    # There are no distance or duration aspects, so this is purely about load.
    # Load should be a no-op, since there's a reload depot immediately after
    # the one we're trying to remove. So delta cost must be 0.
    assert_equal(remove_cost(route[3], data, CostEvaluator([1000], 0, 0)), 0)

    # And similarly removing the second depot should also be a no-op.
    assert_equal(remove_cost(route[4], data, CostEvaluator([1000], 0, 0)), 0)


def test_empty_route_delta_cost_bug():
    """
    Tests that a bug identified in #853 has been fixed. Before fixing this bug,
    inserting or removing a client involving an empty route incorrectly
    included the empty route's costs.
    """
    mat = [
        [0, 5, 1],
        [5, 0, 1],
        [1, 1, 0],
    ]
    data = ProblemData(
        depots=[Depot(x=0, y=0), Depot(x=0, y=0)],
        clients=[Client(x=0, y=0)],
        vehicle_types=[
            VehicleType(1, start_depot=0, end_depot=1, max_duration=0),
        ],
        duration_matrices=[mat],
        distance_matrices=[mat],
    )

    cost_eval = CostEvaluator([], 1, 1)

    # Inserting the client in an empty route results in distance (2) and
    # time warp (2). Before fixing the bug, delta cost also included the empty
    # route's costs (5 distance and 5 time warp).
    route = make_search_route(data, [])
    assert_equal(insert_cost(Node(loc=2), route[0], data, cost_eval), 4)

    # Similarly, if removing a client results in an empty route, then we should
    # not include the empty route's costs.
    route = make_search_route(data, [2])
    assert_equal(remove_cost(route[1], data, cost_eval), -4)

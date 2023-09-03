from typing import List, Optional, Tuple

import numpy as np
import pytest
from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import Client, ProblemData, VehicleType
from pyvrp.search._search import Node, Route
from pyvrp.tests.helpers import make_heterogeneous


@pytest.mark.parametrize("loc", [0, 1, 10])
def test_node_init(loc: int):
    """
    Tests that after initialisation, a Node has index 0 and is not in a route.
    """
    node = Node(loc=loc)
    assert_equal(node.client, loc)
    assert_equal(node.idx, 0)
    assert_(node.route is None)


@pytest.mark.parametrize(("idx", "vehicle_type"), [(0, 0), (1, 0), (1, 1)])
def test_route_init(ok_small, idx: int, vehicle_type: int):
    """
    Tests that after initialisation, a Route has the given index and vehicle
    type.
    """
    data = make_heterogeneous(ok_small, [VehicleType(1, 1), VehicleType(2, 2)])

    route = Route(data, idx=idx, vehicle_type=vehicle_type)
    assert_equal(route.idx, idx)
    assert_equal(route.vehicle_type, vehicle_type)


@pytest.mark.parametrize("loc", [0, 1, 10])
def test_new_nodes_are_not_depots(loc: int):
    """
    Tests that new nodes cannot be depots: to be a depot in this context
    requires route membership, and new nodes are not in routes yet.
    """
    node = Node(loc=loc)
    assert_(not node.is_depot())


def test_insert_and_remove_update_node_idx_and_route_properties(ok_small):
    """
    Tests that after a node is inserted into a route, its index and route
    properties are updated to reflect its new position in the route. After
    the node is removed from the route, these properties revert to their
    defaults.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)

    # After construction, the node is not in a route yet.
    node = Node(loc=1)
    assert_equal(node.idx, 0)
    assert_(node.route is None)

    # Add to the route, test the route and idx properties are updated.
    route.append(node)
    assert_equal(node.idx, 1)
    assert_(node.route is route)

    # Remove and test the node reverts to its initial state.
    del route[1]
    assert_equal(node.idx, 0)
    assert_(node.route is None)


def test_route_depots_are_depots(ok_small):
    """
    Tests that the start and end depot nodes in a route known they are, in
    fact, depots.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)

    for loc in range(2):
        # The depots flank the clients at indices {1, ..., len(route)}. Thus,
        # depots are at indices 0 and len(route) + 1.
        route.append(Node(loc=loc))
        assert_(route[0].is_depot())
        assert_(route[len(route) + 1].is_depot())


def test_route_append_increases_route_len(ok_small):
    """
    Tests that appending nodes to a route increases the route's length.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    assert_equal(len(route), 0)

    node = Node(loc=1)
    route.append(node)
    assert_equal(len(route), 1)
    assert_(route[1] is node)  # pointers, so must be same object

    node = Node(loc=2)
    route.append(node)
    assert_equal(len(route), 2)
    assert_(route[2] is node)  # pointers, so must be same object


def test_route_insert(ok_small):
    """
    Tests that inserting and appending nodes works as expected: appending adds
    to the end, inserting places at the given index.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    assert_equal(len(route), 0)

    # Insert a few nodes so we have an actual route.
    route.append(Node(loc=1))
    route.append(Node(loc=2))
    assert_equal(len(route), 2)
    assert_equal(route[1].client, 1)
    assert_equal(route[2].client, 2)

    # # Now insert a new nodes at index 1.
    route.insert(1, Node(loc=3))
    assert_equal(len(route), 3)
    assert_equal(route[1].client, 3)
    assert_equal(route[2].client, 1)
    assert_equal(route[3].client, 2)


def test_route_iter_returns_all_clients(ok_small):
    """
    Tests that iterating over a route returns all clients in the route, but
    not the depots.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)

    for loc in [1, 2, 3]:
        route.append(Node(loc=loc))

    nodes = [node for node in route]
    assert_equal(len(nodes), len(route))

    # Iterating the Route object returns all clients, not the depots at index
    # ``0`` and index ``len(route) + 1`` in the Route object.
    assert_equal(nodes[0], route[1])
    assert_equal(nodes[1], route[2])
    assert_equal(nodes[2], route[3])


def test_route_add_and_delete_client_leaves_route_empty(ok_small):
    """
    Tests that adding and removing a client leaves a route empty.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)

    route.append(Node(loc=1))
    assert_equal(len(route), 1)

    del route[1]
    assert_equal(len(route), 0)


def test_route_delete_reduces_size_by_one(ok_small):
    """
    Deleting an item at an index removes only the indicated index, not more.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)

    route.append(Node(loc=1))
    route.append(Node(loc=2))
    assert_equal(len(route), 2)

    del route[1]
    assert_equal(len(route), 1)
    assert_equal(route[1].client, 2)


@pytest.mark.parametrize("num_nodes", range(4))
def test_route_clear_empties_entire_route(ok_small, num_nodes: int):
    """
    The clear() method should clear the entire route, not just remove part of
    it.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)

    for loc in range(num_nodes):
        route.append(Node(loc=loc))
        assert_equal(len(route), loc + 1)

    route.clear()
    assert_equal(len(route), 0)


def test_excess_load(ok_small):
    """
    Tests that the route calculations excess load correctly.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    for loc in [1, 2, 3, 4]:
        route.append(Node(loc=loc))
    route.update()

    # The instance has four clients, which have a total demand of 18. The only
    # vehicle type in the instance has a capacity of 10, so this route has
    # excess load.
    assert_equal(route.load(), 18)
    assert_equal(route.capacity(), 10)
    assert_(route.has_excess_load())


@pytest.mark.parametrize("fixed_cost", [0, 9])
def test_fixed_cost(ok_small, fixed_cost: int):
    """
    Tests that the fixed cost method returns the assigned vehicle type's fixed
    cost value.
    """
    data = make_heterogeneous(ok_small, [VehicleType(10, 2, fixed_cost)])
    route = Route(data, idx=0, vehicle_type=0)
    assert_allclose(route.fixed_cost(), fixed_cost)


@pytest.mark.parametrize("client", [1, 2, 3, 4])
def test_dist_and_load_for_single_client_routes(ok_small, client: int):
    """
    Tests that the route calculates distance and load correctly for a
    single-client route.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    route.append(Node(loc=client))
    route.update()

    # Only the client has any demand, so the total route load should be equal
    # to it.
    assert_equal(route.load(), ok_small.client(client).demand)
    assert_equal(route.load_between(0, 2), ok_small.client(client).demand)

    # The load_between() function is inclusive.
    assert_equal(route.load_between(0, 0), 0)
    assert_equal(route.load_between(1, 1), ok_small.client(client).demand)

    # Distances on various segments of the route.
    assert_equal(route.dist_between(0, 1), ok_small.dist(0, client))
    assert_equal(route.dist_between(1, 2), ok_small.dist(client, 0))
    assert_equal(
        route.dist_between(0, 2),
        ok_small.dist(0, client) + ok_small.dist(client, 0),
    )


def test_route_overlaps_with_self_no_matter_the_tolerance_value(ok_small):
    """
    Tests that a route always overlaps with itself.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    route.append(Node(loc=1))
    route.append(Node(loc=2))

    route.update()

    assert_(route.overlaps_with(route, 0))
    assert_(route.overlaps_with(route, 0.5))
    assert_(route.overlaps_with(route, 1))


def test_all_routes_overlap_with_maximum_tolerance_value(ok_small):
    """
    Tests that any route overlaps with any other route with the maximum
    tolerance value.
    """
    route1 = Route(ok_small, idx=0, vehicle_type=0)
    for loc in [1, 2]:
        route1.append(Node(loc=loc))
    route1.update()

    route2 = Route(ok_small, idx=0, vehicle_type=0)
    for loc in [3, 4]:
        route2.append(Node(loc=loc))
    route2.update()

    # The routes are clearly not the same, and don't overlap with zero
    # tolerance.
    assert_(not route1.overlaps_with(route2, 0))
    assert_(not route2.overlaps_with(route1, 0))

    # But with maximum tolerance, they do.
    assert_(route1.overlaps_with(route2, 1))
    assert_(route2.overlaps_with(route1, 1))


def test_data_is_not_updated_until_update_call(ok_small):
    """
    Tests that route statistics like distance, time window segments, and load
    are not updated until ``update()`` has been called.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)

    # Add a new client to the route. update() has not been called, so the route
    # statistics are not correct.
    route.append(Node(loc=1))
    assert_(route.load() != ok_small.client(1).demand)
    assert_(
        route.dist_between(0, 2) != ok_small.dist(0, 1) + ok_small.dist(1, 0)
    )

    # Update. This recalculates the statistics, which should now be correct.
    route.update()
    assert_equal(route.load(), ok_small.client(1).demand)
    assert_equal(
        route.dist_between(0, 2), ok_small.dist(0, 1) + ok_small.dist(1, 0)
    )

    # Same story with another client: incorrect before update, correct after.
    route.append(Node(loc=2))
    assert_(
        route.load() != ok_small.client(1).demand + ok_small.client(2).demand
    )

    route.update()
    assert_equal(
        route.load(), ok_small.client(1).demand + ok_small.client(2).demand
    )
    assert_equal(
        route.dist_between(0, 3),
        ok_small.dist(0, 1) + ok_small.dist(1, 2) + ok_small.dist(2, 0),
    )


@pytest.mark.parametrize("locs", [(1, 2, 3), (3, 4), (1,)])
def test_str_contains_route(ok_small, locs: List[int]):
    """
    Test that each client in the route is also printed in the route's __str__.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)

    for loc in locs:
        route.append(Node(loc=loc))

    for loc in locs:
        assert_(str(loc) in str(route))


def test_route_tws_access(ok_small):
    """
    Tests access to a client's or depot's time window segment, as tracked by
    the route.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    for client in range(1, ok_small.num_clients + 1):
        route.append(Node(loc=client))

    route.update()

    for loc in [0, 1, 2, 3, 4, 5]:  # = [depot, *clients, depot]
        if 0 < loc < len(route) + 1:
            client = ok_small.client(loc)  # if actual client
        else:
            client = ok_small.client(0)  # else depot

        tws = route.tws(loc)
        assert_equal(tws.tw_early(), client.tw_early)
        assert_equal(tws.tw_late(), client.tw_late)
        assert_equal(tws.duration(), client.service_duration)
        assert_equal(tws.total_time_warp(), 0)


@pytest.mark.parametrize("loc", [1, 2, 3, 4])
def test_tws_between_same_client_returns_node_tws(ok_small, loc: int):
    """
    Tests that calling the ``tws_between()`` with the same start and end
    arguments returns a node's time window segment data.
    """
    client = ok_small.client(loc)

    route = Route(ok_small, idx=0, vehicle_type=0)
    route.append(Node(loc=loc))
    route.update()

    # Duration of the depot node TWS's is zero, and for the client it is equal
    # to the service duration.
    assert_equal(route.tws_between(0, 0).duration(), 0)
    assert_equal(route.tws_between(1, 1).duration(), client.service_duration)
    assert_equal(route.tws_between(2, 2).duration(), 0)

    # Single route solutions are all feasible for this instance.
    assert_equal(route.time_warp(), 0)


def test_tws_between_same_as_tws_before_after_when_one_side_is_depot(ok_small):
    """
    Tests that ``tws_between()`` returns the same value as ``tws_before()`` or
    ``tws_after()`` when one side is the depot.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    for client in range(1, ok_small.num_clients + 1):
        route.append(Node(loc=client))

    route.update()

    for idx in [1, 2, 3, 4]:
        before = route.tws_before(idx)
        between_before = route.tws_between(0, idx)
        assert_equal(before.duration(), between_before.duration())
        assert_equal(
            before.total_time_warp(), between_before.total_time_warp()
        )

        after = route.tws_after(idx)
        between_after = route.tws_between(idx, len(route) + 1)
        assert_equal(after.duration(), between_after.duration())
        assert_equal(after.total_time_warp(), between_after.total_time_warp())


def test_tws_between_single_route_solution_has_correct_time_warp(ok_small):
    """
    Tests time window segment access on a single-route solution where we know
    exactly where in the route time warp occurs.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    for client in range(1, ok_small.num_clients + 1):
        route.append(Node(loc=client))

    assert_equal(len(route), ok_small.num_clients)

    route.update()
    assert_(route.has_time_warp())
    assert_equal(route.tws_between(0, 5).total_time_warp(), route.time_warp())

    # Client #1 (at idx 1) causes the time warp in combination with client #3:
    # #1 can only be visited after #3's window has already closed.
    assert_equal(route.time_warp(), 3_633)
    assert_equal(route.tws_between(1, 4).total_time_warp(), 3_633)
    assert_equal(route.tws_between(0, 4).total_time_warp(), 3_633)
    assert_equal(route.tws_between(1, 5).total_time_warp(), 3_633)
    assert_equal(route.tws_between(1, 3).total_time_warp(), 3_633)

    # But excluding client #1, other subtours are (time-)feasible:
    for start, end in [(2, 4), (3, 5), (2, 3), (4, 5), (5, 5), (0, 1), (0, 2)]:
        assert_equal(route.tws_between(start, end).total_time_warp(), 0)


def test_distance_is_equal_to_dist_between_over_whole_route(ok_small):
    """
    Tests that calling distance() on the route object is the same as calling
    dist_between() with the start and end depot indices as arguments.
    """
    route = Route(ok_small, idx=0, vehicle_type=0)
    for client in range(1, ok_small.num_clients + 1):
        route.append(Node(loc=client))
    route.update()

    assert_equal(route.distance(), route.dist_between(0, len(route) + 1))


@pytest.mark.parametrize(
    ("shift_tw", "expected_tw"),
    [
        ((None, None), (0, 1000)),  # not set; should default to depot
        ((0, 1000), (0, 1000)),  # same as depot
        ((0, 500), (0, 500)),  # earlier tw_late, should lower tw_late
        ((250, 1000), (250, 1000)),  # later tw_early, should increase tw_early
        ((300, 600), (300, 600)),  # both more restricitve
    ],
)
def test_shift_duration_depot_time_window_interaction(
    shift_tw: Tuple[Optional[int], Optional[int]],
    expected_tw: Tuple[int, int],
):
    """
    Tests that the route's depot time window is restricted to the most
    restrictive of [depot early, depot late] and [shift early, shift late].
    The depot time window defaults to [0, 1_000], and the shift time window
    varies around that.
    """
    data = ProblemData(
        clients=[Client(x=0, y=0, tw_early=0, tw_late=1_000)],
        vehicle_types=[
            VehicleType(0, 1, tw_early=shift_tw[0], tw_late=shift_tw[1])
        ],
        distance_matrix=np.zeros((1, 1), dtype=int),
        duration_matrix=np.zeros((1, 1), dtype=int),
    )

    route = Route(data, idx=0, vehicle_type=0)
    assert_equal(len(route), 0)

    for idx in [0, 1]:
        tws = route.tws(idx)
        assert_allclose(tws.tw_early(), expected_tw[0])
        assert_allclose(tws.tw_late(), expected_tw[1])

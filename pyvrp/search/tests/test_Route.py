from typing import List

import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import VehicleType
from pyvrp.search._search import Node, Route
from pyvrp.tests.helpers import make_heterogeneous, read


@pytest.mark.parametrize("loc", [0, 1, 10])
def test_node_init(loc: int):
    node = Node(loc=loc)
    assert_equal(node.client, loc)
    assert_equal(node.idx, 0)
    assert_(node.route is None)


@pytest.mark.parametrize(("idx", "vehicle_type"), [(0, 0), (1, 0), (1, 1)])
def test_route_init(idx: int, vehicle_type: int):
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, [VehicleType(1, 1), VehicleType(2, 2)])

    route = Route(data, idx=idx, vehicle_type=vehicle_type)
    assert_equal(route.idx, idx)
    assert_equal(route.vehicle_type, vehicle_type)


@pytest.mark.parametrize("loc", [0, 1, 10])
def test_new_nodes_are_not_depots(loc: int):
    node = Node(loc=loc)
    assert_(not node.is_depot())


def test_route_depots_are_depots():
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)

    for loc in range(2):
        # The depots flank the clients at indices {1, ..., len(route)}. Thus,
        # depots are at indices 0 and len(route) + 1.
        route.append(Node(loc=loc))
        assert_(route[0].is_depot())
        assert_(route[len(route) + 1].is_depot())


def test_route_append_increases_route_len():
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)
    assert_equal(len(route), 0)

    node = Node(loc=1)
    route.append(node)
    assert_equal(len(route), 1)
    assert_(route[1] is node)  # pointers, so must be same object

    node = Node(loc=2)
    route.append(node)
    assert_equal(len(route), 2)
    assert_(route[2] is node)  # pointers, so must be same object


def test_route_insert():
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)
    assert_equal(len(route), 0)

    # Insert a few customers so we have an actual route.
    route.append(Node(loc=1))
    route.append(Node(loc=2))
    assert_equal(len(route), 2)
    assert_equal(route[1].client, 1)
    assert_equal(route[2].client, 2)

    # # Now insert a new customer at index 1.
    route.insert(1, Node(loc=3))
    assert_equal(len(route), 3)
    assert_equal(route[1].client, 3)
    assert_equal(route[2].client, 1)
    assert_equal(route[3].client, 2)


def test_route_iter_returns_all_clients():
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)

    for loc in [1, 2, 3]:
        route.append(Node(loc=loc))

    nodes = [node for node in route]
    assert_equal(len(nodes), len(route))

    # Iterating the Route object returns all clients, not the depots at index
    # ``0`` and index ``len(route) + 1`` in the Route object.
    assert_equal(nodes[0], route[1])
    assert_equal(nodes[1], route[2])
    assert_equal(nodes[2], route[3])


def test_route_add_and_remove_client_leaves_route_empty():
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)

    route.append(Node(loc=1))
    assert_equal(len(route), 1)

    route.remove(1)
    assert_equal(len(route), 0)


def test_route_remove_reduces_size_by_one():
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)

    route.append(Node(loc=1))
    route.append(Node(loc=2))
    assert_equal(len(route), 2)

    route.remove(1)
    assert_equal(len(route), 1)


@pytest.mark.parametrize("num_nodes", range(4))
def test_route_clear_empties_entire_route(num_nodes: int):
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)

    for loc in range(num_nodes):
        route.append(Node(loc=loc))
        assert_equal(len(route), loc + 1)

    route.clear()
    assert_equal(len(route), 0)


def test_excess_load():
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)

    for loc in [1, 2, 3, 4]:
        route.append(Node(loc=loc))

    route.update()

    # The instance has four clients, which have a total demand of 18. The only
    # vehicle type in the instance has a capacity of 10, so this route has
    # excess load.
    assert_equal(route.load(), 18)
    assert_equal(route.capacity(), 10)
    assert_(route.has_excess_load())


@pytest.mark.parametrize("client", [1, 2, 3, 4])
def test_dist_and_load_for_single_client_routes(client: int):
    data = read("data/OkSmall.txt")

    route = Route(data, idx=0, vehicle_type=0)
    route.append(Node(loc=client))
    route.update()

    # Only the client has any demand, so the total route load should be equal
    # to it.
    assert_equal(route.load(), data.client(client).demand)
    assert_equal(route.load_between(0, 2), data.client(client).demand)

    # The load_between() function is inclusive.
    assert_equal(route.load_between(0, 0), 0)
    assert_equal(route.load_between(1, 1), data.client(client).demand)

    # Distances on various segments of the route.
    assert_equal(route.dist_between(0, 1), data.dist(0, client))
    assert_equal(route.dist_between(1, 2), data.dist(client, 0))
    assert_equal(
        route.dist_between(0, 2), data.dist(0, client) + data.dist(client, 0)
    )


def test_route_overlaps_with_self_no_matter_the_tolerance_value():
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)
    route.append(Node(loc=1))
    route.append(Node(loc=2))

    route.update()

    assert_(route.overlaps_with(route, 0))
    assert_(route.overlaps_with(route, 0.5))
    assert_(route.overlaps_with(route, 1))


def test_all_routes_overlap_with_maximum_tolerance_value():
    data = read("data/OkSmall.txt")

    route1 = Route(data, idx=0, vehicle_type=0)
    for loc in [1, 2]:
        route1.append(Node(loc=loc))
    route1.update()

    route2 = Route(data, idx=0, vehicle_type=0)
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


# TODO test overlap with less extreme cases, including wrap around etc.


@pytest.mark.parametrize("locs", [(1, 2, 3), (3, 4), (1,)])
def test_str_contains_route(locs: List[int]):
    data = read("data/OkSmall.txt")
    route = Route(data, idx=0, vehicle_type=0)

    for loc in locs:
        route.append(Node(loc=loc))

    for loc in locs:
        assert_(str(loc) in str(route))


# TODO test time windows

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


@pytest.mark.parametrize("loc", [0, 1, 10])
def test_new_nodes_are_not_depots(loc: int):
    node = Node(loc=loc)
    assert_(not node.is_depot())


@pytest.mark.parametrize(("idx", "vehicle_type"), [(0, 0), (1, 0), (1, 1)])
def test_route_init(idx: int, vehicle_type: int):
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, [VehicleType(1, 1), VehicleType(2, 2)])

    route = Route(data, idx=idx, vehicle_type=vehicle_type)
    assert_equal(route.idx, idx)
    assert_equal(route.vehicle_type, vehicle_type)

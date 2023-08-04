import pytest
from numpy.testing import assert_, assert_equal

from pyvrp.search._search import Node, Route
from pyvrp.tests.helpers import read


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


@pytest.mark.parametrize("idx", [0, 5, 10])
def test_route_idx(idx: int):
    data = read("data/OkSmall.txt")
    route = Route(data, idx=idx, veh_type=0)
    assert_equal(route.idx, idx)

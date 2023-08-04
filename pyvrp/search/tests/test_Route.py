import pytest
from numpy.testing import assert_, assert_equal

from pyvrp.search._search import Node


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

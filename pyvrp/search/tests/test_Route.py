from numpy.testing import assert_, assert_equal

from pyvrp.search._search import Node


def test_node_init_not_in_route():
    node = Node(loc=10)
    assert_equal(node.client, 10)
    assert_equal(node.idx, 0)
    assert_(node.route is None)

from numpy.testing import assert_, assert_equal

from pyvrp.search._search import Node


def test_node_init():
    node = Node(loc=10)
    assert_equal(node.client, 10)
    assert_equal(node.idx, 0)
    assert_(node.route is None)


def test_new_nodes_are_not_depots():
    node = Node(loc=10)
    assert_(not node.is_depot())

    node = Node(loc=0)
    assert_(not node.is_depot())

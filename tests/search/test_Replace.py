from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import Replace
from pyvrp.search._search import Node
from tests.helpers import make_search_route


def test_move_delta_cost(ok_small_prizes):
    data = ok_small_prizes
    op = Replace(ok_small_prizes)

    route = make_search_route(data, [2])
    node = Node(3)

    old = 1944 + 1965  # [0, 2, 0]
    new = 1931 + 2063  # [0, 3, 0]
    diff = new - old
    diff += 15 - 10  # prize_diff

    cost_eval = CostEvaluator([20], 6, 6)
    assert_equal(op.evaluate(route[1], node, cost_eval), diff)

    # swap 3 and 2
    op.apply(route[1], node)
    route.update()

    assert_equal(len(route), 3)
    assert_equal(route[1].client, 3)


def test_move_required_client(ok_small_prizes):
    pass


def test_move_groups(ok_small_mutually_exclusive_groups):
    pass


def test_supports(ok_small, ok_small_prizes):
    """
    Tests that Replace only supports instance with optional clients.
    """
    assert_(not Replace.supports(ok_small))  # no optional clients
    assert_(Replace.supports(ok_small_prizes))  # has optional clients

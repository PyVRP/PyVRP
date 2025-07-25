from numpy.testing import assert_equal

from pyvrp import (
    CostEvaluator,
)
from pyvrp.search import (
    SwapInPlace0,
    SwapInPlace1,
    SwapInPlace2,
)
from pyvrp.search._search import Node
from tests.helpers import make_search_route


def test_swap_in_place_0(ok_small_prizes):
    data = ok_small_prizes
    op = SwapInPlace0(ok_small_prizes)

    route = make_search_route(data, [2])
    node = Node(3)

    # insert 3 before 2
    op.apply(route[1], node)
    route.update()

    assert_equal(len(route), 4)
    assert_equal(route[1].client, 3)
    assert_equal(route[2].client, 2)


def test_swap_in_place_1(ok_small_prizes):
    data = ok_small_prizes
    op = SwapInPlace1(ok_small_prizes)

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


def test_swap_in_place_2(ok_small_prizes):
    data = ok_small_prizes
    op = SwapInPlace2(ok_small_prizes)
    cost_eval = CostEvaluator([20], 6, 6)

    route = make_search_route(data, [2])
    node = Node(3)

    delta_cost = op.evaluate(route[1], node, cost_eval)
    assert_equal(delta_cost, 0)  # contains depot

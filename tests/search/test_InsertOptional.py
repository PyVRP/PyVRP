from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import InsertOptional
from pyvrp.search._search import Node
from tests.helpers import make_search_route


def test_inserts_when_makes_sense(prize_collecting):
    """
    Tests that InsertOptional inserts optional clients when that is an
    improving move.
    """
    route = make_search_route(prize_collecting, [1, 2, 3])
    assert_equal(str(route), "1 2 3")

    node = Node(loc=6)
    op = InsertOptional(prize_collecting)
    cost_eval = CostEvaluator([0], 0, 0)

    # dist - prize = dist(3, 6) + dist(6, 0) - dist(3, 0) - prize
    #              = 353 + 114 - 325 - 150
    #              = -8.
    delta, should_apply = op.evaluate(node, route[3], cost_eval)
    assert_equal(delta, -8)
    assert_(should_apply)

    op.apply(node, route[3])
    assert_equal(str(route), "1 2 3 6")


def test_skips_assigned_or_missing(ok_small_prizes):
    """
    Tests that InsertOptional skips assigned clients, or when the other client
    is missing.
    """
    route = make_search_route(ok_small_prizes, [1, 2])
    assert_(route[1].route)
    assert_(route[2].route)

    # Client 1 is already assigned, cannot be inserted again.
    op = InsertOptional(ok_small_prizes)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], route[2], cost_eval), (0, False))

    # These are not assigned anywhere, so cannot insert after.
    node3 = Node(loc=3)
    node4 = Node(loc=4)
    assert_equal(op.evaluate(node3, node4, cost_eval), (0, False))


def test_supports(
    ok_small_prizes,
    ok_small,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that InsertOptional supports instances with optional clients.
    """
    assert_(not InsertOptional.supports(ok_small))
    assert_(InsertOptional.supports(ok_small_prizes))
    assert_(InsertOptional.supports(ok_small_mutually_exclusive_groups))


def test_group_skip_required():
    """
    TODO
    """
    pass


def test_group_skip_duplicate():
    """
    TODO
    """
    pass

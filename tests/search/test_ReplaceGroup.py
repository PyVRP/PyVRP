from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp import Solution as PyVRPSolution
from pyvrp.search import ReplaceGroup
from pyvrp.search._search import Node, Solution


def test_replace(ok_small_mutually_exclusive_groups):
    """
    Tests that ReplaceGroup replace clients within the same group.
    """
    data = ok_small_mutually_exclusive_groups
    cost_eval = CostEvaluator([0], 0, 0)

    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[1, 4]]))
    assert_equal(str(sol.routes[0]), "1 4")

    op = ReplaceGroup(data)
    op.init(sol)

    # Replacing client 1 with 2 is slightly improving:
    # delta = dist(0, 2) + dist(2, 4) - dist(0, 1) - dist(1, 4)
    #       = 1944 + 1090 - 1544 - 1593
    #       = -103.
    node = Node(loc=2)
    assert_equal(op.evaluate(node, cost_eval), (-103, True))

    op.apply(node)
    assert_equal(str(sol.routes[0]), "2 4")


def test_supports(
    ok_small,
    ok_small_prizes,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that ReplaceGroup only supports instances with groups.
    """
    assert_(ReplaceGroup.supports(ok_small_mutually_exclusive_groups))
    assert_(not ReplaceGroup.supports(ok_small))
    assert_(not ReplaceGroup.supports(ok_small_prizes))

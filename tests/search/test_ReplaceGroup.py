import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp import (
    Client,
    ClientGroup,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    VehicleType,
)
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
    sol.load(PyVRPSolution(data, [[0, 3]]))
    assert_equal(str(sol.routes[0]), "C0 C3")

    op = ReplaceGroup(data)
    op.init(sol)

    # Replacing C0 with C1 is slightly improving:
    # delta = dist(D0, C1) + dist(C1, C3) - dist(D0, C0) - dist(C0, C3)
    #       = 1944 + 1090 - 1544 - 1593
    #       = -103.
    node = Node("C1")
    assert_equal(op.evaluate(node, cost_eval), (-103, True))

    op.apply(node)
    assert_equal(str(sol.routes[0]), "C1 C3")


def test_replace_accounts_for_prizes():
    """
    Tests that prizes are included in the replacement cost.
    """
    matrix = np.zeros((1, 1), dtype=int)
    data = ProblemData(
        locations=[Location(0, 0)],
        clients=[
            Client(0, prize=1, required=False, group=0),
            Client(0, prize=5, required=False, group=0),
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(1)],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
        groups=[ClientGroup([0, 1])],
    )
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0]]))

    op = ReplaceGroup(data)
    op.init(sol)

    # Replacing C0 with C1 collects four more units of prize.
    move = op.evaluate(sol.nodes[1], CostEvaluator([], 0, 0))
    assert_equal(move, (-4, True))


def test_supports(
    gtsp,
    ok_small,
    ok_small_prizes,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that ReplaceGroup only supports instances with mutually exclusive
    client groups.
    """
    assert_(ReplaceGroup.supports(gtsp))
    assert_(ReplaceGroup.supports(ok_small_mutually_exclusive_groups))
    assert_(not ReplaceGroup.supports(ok_small))
    assert_(not ReplaceGroup.supports(ok_small_prizes))

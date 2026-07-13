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
from pyvrp.search import RelocateWithAlternative
from pyvrp.search._search import Solution


def test_relocates_with_alternative(ok_small_mutually_exclusive_groups):
    """
    Tests that RelocateWithAlternative selects the best group alternative.
    """
    data = ok_small_mutually_exclusive_groups
    cost_eval = CostEvaluator([0], 0, 0)

    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0, 3]]))
    assert_equal(str(sol.routes[0]), "C0 C3")

    op = RelocateWithAlternative(data)
    op.init(sol)

    # Replaces C0 by the best group alternative, C2, and moves it after C3.
    # The old route costs 1544 + 1593 + 1475 = 4612. The proposed route
    # C3 -> C2 costs 1476 + 828 + 2063 = 4367, for a delta of -245.
    move = op.evaluate(sol.nodes[0], sol.nodes[3], cost_eval)
    assert_equal(move, (-245, True))

    op.apply(sol.nodes[0], sol.nodes[3])
    assert_equal(str(sol.routes[0]), "C3 C2")


def test_accounts_for_prizes_and_fixed_vehicle_costs():
    """
    Tests that prizes and fixed vehicle costs are included in the move cost.
    """
    matrix = np.zeros((1, 1), dtype=int)
    data = ProblemData(
        locations=[Location(0, 0)],
        clients=[
            Client(0, prize=1, required=False, group=0),
            Client(0, prize=5, required=False, group=0),
            Client(0),
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(2, fixed_cost=100)],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
        groups=[ClientGroup([0, 1], required=True)],
    )
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0], [2]]))

    op = RelocateWithAlternative(data)
    op.init(sol)

    # Moving to the second route saves one fixed vehicle cost (100), and the
    # alternative collects four more units of prize.
    move = op.evaluate(sol.nodes[0], sol.nodes[2], CostEvaluator([], 0, 0))
    assert_equal(move, (-104, True))


def test_supports(
    ok_small, ok_small_prizes, ok_small_mutually_exclusive_groups
):
    """
    Tests that RelocateWithAlternative only supports data with client groups.
    """
    op = RelocateWithAlternative
    assert_(op.supports(ok_small_mutually_exclusive_groups))
    assert_(not op.supports(ok_small))
    assert_(not op.supports(ok_small_prizes))

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
from pyvrp.search import RelocateAlternative
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_relocate_alternative_within_route(ok_small_mutually_exclusive_groups):
    """
    Tests RelocateAlternative within the same route.
    """
    data = ok_small_mutually_exclusive_groups
    cost_eval = CostEvaluator([0], 0, 0)

    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0, 3]]))
    assert_equal(str(sol.routes[0]), "C0 C3")

    op = RelocateAlternative(data)
    op.init(sol)

    # C1 is the first improving alternative of C0:
    # delta = dist(D0, C3) + dist(C3, C1) + dist(C1, D0)
    #       - dist(D0, C0) - dist(C0, C3) - dist(C3, D0)
    #       = 1476 + 1090 + 1965 - 1544 - 1593 - 1475
    #       = -81.
    move = op.evaluate(sol.clients[0], sol.clients[3], cost_eval)
    assert_equal(move, (-81, True))

    op.apply(sol.clients[0], sol.clients[3])
    assert_equal(str(sol.routes[0]), "C3 C1")


def test_relocate_alternative_between_routes(
    ok_small_mutually_exclusive_groups,
):
    """
    Tests RelocateAlternative between two routes.
    """
    data = ok_small_mutually_exclusive_groups
    cost_eval = CostEvaluator([0], 0, 0)

    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0], [3]]))
    assert_equal(str(sol.routes[0]), "C0")
    assert_equal(str(sol.routes[1]), "C3")

    op = RelocateAlternative(data)
    op.init(sol)

    # C1 is the first improving alternative of C0:
    # delta = dist(C3, C1) + dist(C1, D0) - dist(C3, D0)
    #       - dist(D0, C0) - dist(C0, D0)
    #       = 1090 + 1965 - 1475 - 1544 - 1726
    #       = -1690.
    move = op.evaluate(sol.clients[0], sol.clients[3], cost_eval)
    assert_equal(move, (-1690, True))

    op.apply(sol.clients[0], sol.clients[3])
    sol.routes[0].update()
    sol.routes[1].update()

    assert_equal(sol.routes[0].num_clients(), 0)
    assert_equal(str(sol.routes[1]), "C3 C1")


def test_accounts_for_prizes_and_fixed_vehicle_costs():
    """
    Tests that prizes and fixed vehicle costs are included in the move cost.
    """
    matrix = np.zeros((1, 1), dtype=int)
    data = ProblemData(
        locations=[Location(0, 0)],
        clients=[
            Client(0, prize=100, required=False, group=0),
            Client(0, prize=250, required=False, group=0),
            Client(0),
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(2, fixed_cost=100)],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
        groups=[ClientGroup([0, 1], required=True)],
    )

    # First scenario, with C0 and C2 on separate routes.
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0], [2]]))

    op = RelocateAlternative(data)
    op.init(sol)

    # Moving to the second route saves one fixed vehicle cost (100), and the
    # alternative collects 150 more units of prize.
    move = op.evaluate(sol.clients[0], sol.clients[2], CostEvaluator([], 0, 0))
    assert_equal(move, (-250, True))

    # Second scenario, with C0 and C2 on the same route.
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0, 2]]))
    op.init(sol)

    empty = sol.routes[1]
    assert_equal(empty.num_clients(), 0)

    # Moving to the empty second route adds one fixed vehicle cost (100),
    # while the cheaper alternative collects 150 more units of prize.
    move = op.evaluate(sol.clients[0], empty[0], CostEvaluator([], 0, 0))
    assert_equal(move, (-50, True))


def test_skips_client_not_in_group(ok_small_mutually_exclusive_groups):
    """
    Tests that the operator skips clients that are not in a group.
    """
    data = ok_small_mutually_exclusive_groups
    assert_(not data.client(3).group)

    sol = Solution(data)
    op = RelocateAlternative(data)
    op.init(sol)

    # C3 is not in a group, so the operator should skip it.
    route = make_search_route(data, ["C3", "C0"])
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], route[2], cost_eval), (0, False))


def test_supports(
    gtsp,
    ok_small,
    ok_small_prizes,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that RelocateAlternative only supports data with mutually exclusive
    client groups.
    """
    assert_(RelocateAlternative.supports(gtsp))
    assert_(RelocateAlternative.supports(ok_small_mutually_exclusive_groups))
    assert_(not RelocateAlternative.supports(ok_small))
    assert_(not RelocateAlternative.supports(ok_small_prizes))

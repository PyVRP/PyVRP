import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import (
    Activity,
    Client,
    CostEvaluator,
    Depot,
    Location,
    MutuallyExclusiveGroup,
    ProblemData,
    Route,
    VehicleType,
)
from pyvrp import Solution as PyVRPSolution
from pyvrp.search import ReplaceGroup
from pyvrp.search._search import Solution


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
    node = sol.clients[1]
    assert_allclose(op.evaluate(node, cost_eval), (-103, True))

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
        groups=[MutuallyExclusiveGroup([Activity("C0"), Activity("C1")])],
    )
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0]]))

    op = ReplaceGroup(data)
    op.init(sol)

    # Replacing C0 with C1 collects four more units of prize.
    move = op.evaluate(sol.clients[1], CostEvaluator([], 0, 0))
    assert_allclose(move, (-4, True))


def test_supports(
    gtsp,
    ok_small,
    ok_small_prizes,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that ReplaceGroup only supports instances with mutually exclusive
    groups.
    """
    assert_(ReplaceGroup.supports(gtsp))
    assert_(ReplaceGroup.supports(ok_small_mutually_exclusive_groups))
    assert_(not ReplaceGroup.supports(ok_small))
    assert_(not ReplaceGroup.supports(ok_small_prizes))


def test_replace_client_with_shipment(mixed_group):
    """
    Replace the client with pickup and insert delivery later in the route.
    """
    matrix = np.array(mixed_group.distance_matrix(0))
    matrix[3, 4] = 100  # delivery directly after pickup is too expensive
    data = mixed_group.replace(distance_matrices=[matrix])
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0, 1]]))
    op = ReplaceGroup(data)
    op.init(sol)
    pickup, _ = sol.shipments[0]
    assert_allclose(op.evaluate(pickup, CostEvaluator([], 0, 0)), (-14, True))
    op.apply(pickup)
    assert_equal(str(sol.routes[0]), "L0 C1 U0")
    assert_(sol.clients[0].route is None)


def test_replace_client_with_shipment_does_not_cross_depot(mixed_group):
    """
    Delivery cannot be inserted beyond the pickup's trip.
    """
    matrix = np.array(mixed_group.distance_matrix(0))
    matrix[3, 4] = 100
    data = mixed_group.replace(distance_matrices=[matrix])
    sol = Solution(data)
    route = Route(data, map(Activity, ["C0", "D0", "C1"]), 0)
    sol.load(PyVRPSolution(data, [route]))
    op = ReplaceGroup(data)
    op.init(sol)
    assert_allclose(
        op.evaluate(sol.shipments[0][0], CostEvaluator([], 0, 0)), (0, False)
    )


def test_replace_shipment_with_client(mixed_group):
    """
    Try delivery when placing the client at pickup does not improve.
    """
    matrix = np.array(mixed_group.distance_matrix(0))
    matrix[1, 2] = 100
    data = mixed_group.replace(
        clients=[Client(1, prize=100, required=False, group=0), Client(2)],
        distance_matrices=[matrix],
    )
    sol = Solution(data)
    route = Route(data, map(Activity, ["L0", "C1", "U0"]), 0)
    sol.load(PyVRPSolution(data, [route]))
    op = ReplaceGroup(data)
    op.init(sol)
    client = sol.clients[0]
    assert_allclose(op.evaluate(client, CostEvaluator([], 0, 0)), (-81, True))
    op.apply(client)
    assert_equal(str(sol.routes[0]), "C1 C0")
    pickup, delivery = sol.shipments[0]
    assert_(pickup.route is None and delivery.route is None)


def test_replace_shipment_with_shipment(mixed_group):
    """
    Keep both shipment nodes at their existing positions.
    """
    sol = Solution(mixed_group)
    route = Route(mixed_group, map(Activity, ["L0", "C1", "U0"]), 0)
    sol.load(PyVRPSolution(mixed_group, [route]))
    op = ReplaceGroup(mixed_group)
    op.init(sol)
    pickup, _ = sol.shipments[1]
    assert_allclose(op.evaluate(pickup, CostEvaluator([], 0, 0)), (-10, True))
    op.apply(pickup)
    assert_equal(str(sol.routes[0]), "L1 C1 U1")

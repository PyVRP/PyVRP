import numpy as np
import pytest
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
from pyvrp.search import RelocateAlternative, ReplaceGroup
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_relocate_client_to_shipment(mixed_group):
    """
    Remove a client and relocate its shipment alternative after V.
    """
    matrix = np.array(mixed_group.distance_matrix(0))
    matrix[3, 4] = 100
    data = mixed_group.replace(
        clients=[*mixed_group.clients(), Client(0)],
        distance_matrices=[matrix],
    )
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0, 1, 2]]))
    op = RelocateAlternative(data)
    op.init(sol)
    U, V = sol.clients[0], sol.clients[1]
    assert_allclose(op.evaluate(U, V, CostEvaluator([], 0, 0)), (-13, True))
    op.apply(U, V)
    assert_equal(str(sol.routes[0]), "C1 L0 C2 U0")
    assert_(U.route is None)


@pytest.mark.parametrize(
    ("visits", "v_pos", "expected"),
    [
        ("L0 U0 C1 C2", 3, "C1 C0 C2"),
        ("C1 C2 L0 U0", 1, "C1 C0 C2"),
        ("L0 C1 U0 C2", 4, "C1 C2 C0"),
        ("L0 C1 U0 C2", 2, "C1 C0 C2"),
        ("L0 C1 C2 U0", 2, "C1 C0 C2"),
        ("C1 C2 L0 C3 U0", 1, "C1 C0 C2 C3"),
    ],
)
def test_relocate_shipment_to_client(mixed_group, visits, v_pos, expected):
    """
    Remove both shipment nodes and relocate the client after V.
    """
    matrix = np.random.default_rng(42).integers(1, 3, size=(7, 7))
    np.fill_diagonal(matrix, 0)
    data = mixed_group.replace(
        clients=[
            Client(1, prize=100, required=False, group=0),
            Client(2),
            Client(0),
            Client(1),
        ],
        distance_matrices=[matrix],
    )
    cost_eval = CostEvaluator([], 0, 0)
    initial = PyVRPSolution(
        data, [Route(data, map(Activity, visits.split()), 0)]
    )
    improved = PyVRPSolution(
        data, [Route(data, map(Activity, expected.split()), 0)]
    )
    delta = cost_eval.penalised_cost(improved) - cost_eval.penalised_cost(
        initial
    )
    assert_(delta < 0)

    sol = Solution(data)
    sol.load(initial)
    op = RelocateAlternative(data)
    op.init(sol)
    pickup, delivery = sol.shipments[0]
    V = sol.routes[0][v_pos]
    assert_allclose(op.evaluate(pickup, V, cost_eval), (delta, True))
    op.apply(pickup, V)
    sol.routes[0].update()
    assert_equal(str(sol.routes[0]), expected)
    assert_equal(sol.clients[0].pos, V.pos + 1)
    assert_(pickup.route is None and delivery.route is None)
    assert_allclose(
        cost_eval.penalised_cost(sol.unload()),
        cost_eval.penalised_cost(improved),
    )


@pytest.mark.parametrize(
    ("visits", "v_pos", "expected"),
    [
        ("L0 C1 U0 C2", 2, "C1 L1 U1 C2"),
        ("L0 C1 C2 U0", 2, "C1 L1 C2 U1"),
        ("C1 C2 L0 U0", 0, "L1 C1 C2 U1"),
        ("C1 C2 L0 C3 U0", 1, "C1 L1 C2 C3 U1"),
        ("C3 D0 C1 C2 L0 U0", 2, "C3 D0 L1 C1 C2 U1"),
        ("L0 U0 C1 C2", 3, "C1 L1 U1 C2"),
        ("L0 C1 U0 C2", 4, "C1 C2 L1 U1"),
        ("C1 L0 U0 C2 C3", 4, "C1 C2 L1 U1 C3"),
        ("C1 L0 C2 U0 C3", 5, "C1 C2 C3 L1 U1"),
        ("C3 D0 L0 C1 U0 C2", 6, "C3 D0 C1 C2 L1 U1"),
    ],
)
def test_relocate_shipment_within_route(mixed_group, visits, v_pos, expected):
    """
    Insert pickup after V, keeping delivery in place when it follows pickup.
    """
    matrix = np.random.default_rng(42).integers(1, 3, size=(7, 7))
    np.fill_diagonal(matrix, 0)
    data = mixed_group.replace(
        clients=[*mixed_group.clients(), Client(0), Client(1)],
        distance_matrices=[matrix],
    )
    cost_eval = CostEvaluator([], 0, 0)
    initial = PyVRPSolution(
        data, [Route(data, map(Activity, visits.split()), 0)]
    )
    improved = PyVRPSolution(
        data, [Route(data, map(Activity, expected.split()), 0)]
    )
    delta = cost_eval.penalised_cost(improved) - cost_eval.penalised_cost(
        initial
    )
    assert_(delta < 0)

    sol = Solution(data)
    sol.load(initial)
    op = RelocateAlternative(data)
    op.init(sol)
    pickup, delivery = sol.shipments[0]
    delivery_pos = delivery.pos
    V = sol.routes[0][v_pos]
    assert_allclose(op.evaluate(pickup, V, cost_eval), (delta, True))
    op.apply(pickup, V)
    sol.routes[0].update()

    assert_equal(str(sol.routes[0]), expected.replace("D0", "|"))
    new_pickup, new_delivery = sol.shipments[1]
    assert_equal(new_pickup.pos, V.pos + 1)
    assert_(new_delivery.pos > new_pickup.pos)
    if v_pos < delivery_pos:
        assert_equal(new_delivery.pos, delivery_pos)
    else:
        assert_equal(new_delivery.pos, new_pickup.pos + 1)
    assert_(pickup.route is None and delivery.route is None)
    assert_allclose(
        cost_eval.penalised_cost(sol.unload()),
        cost_eval.penalised_cost(improved),
    )


@pytest.mark.parametrize(
    ("visits", "source", "alternative"),
    [
        ("C1 C0", "C0", "L0"),
        ("C1 L0 U0", "L0", "L1"),
        ("C1 L0 C2 U0", "L0", "L1"),
    ],
)
def test_in_place_replacements_use_replace_group(
    mixed_group, visits, source, alternative
):
    """
    RelocateAlternative skips moves that ReplaceGroup evaluates in place.
    """
    data = mixed_group.replace(clients=[*mixed_group.clients(), Client(0)])
    sol = Solution(data)
    sol.load(
        PyVRPSolution(data, [Route(data, map(Activity, visits.split()), 0)])
    )
    U = sol[Activity(source)]
    V = sol.clients[1]
    cost_eval = CostEvaluator([], 0, 0)
    op = RelocateAlternative(data)
    op.init(sol)
    assert_allclose(op.evaluate(U, V, cost_eval), (0, False))

    replacement = ReplaceGroup(data)
    replacement.init(sol)
    node = sol[Activity(alternative)]
    cost_before = cost_eval.penalised_cost(sol.unload())
    delta, improving = replacement.evaluate(node, cost_eval)
    assert_(improving)
    replacement.apply(node)
    sol.routes[0].update()
    assert_equal(node.pos, V.pos + 1)
    assert_(U.route is None)
    assert_allclose(
        cost_eval.penalised_cost(sol.unload()), cost_before + delta
    )


def test_relocate_shipment_does_not_search_delivery_positions(mixed_group):
    """
    Reject a replacement when it only improves by relocating the delivery.
    """
    matrix = np.array(mixed_group.distance_matrix(0))
    matrix[5, 6] = 100
    data = mixed_group.replace(
        clients=[*mixed_group.clients(), Client(0)],
        distance_matrices=[matrix],
    )
    sol = Solution(data)
    route = Route(data, map(Activity, ["L0", "C1", "U0", "C2"]), 0)
    sol.load(PyVRPSolution(data, [route]))
    op = RelocateAlternative(data)
    op.init(sol)
    pickup, _ = sol.shipments[0]
    cost_eval = CostEvaluator([], 0, 0)
    assert_allclose(op.evaluate(pickup, sol.clients[1], cost_eval), (0, False))
    assert_equal(str(sol.routes[0]), "L0 C1 U0 C2")


def test_relocate_shipment_does_not_cross_depot(mixed_group):
    """
    Moving delivery after V cannot move it into a different trip.
    """
    data = mixed_group.replace(clients=[*mixed_group.clients(), Client(0)])
    sol = Solution(data)
    route = Route(data, map(Activity, ["L0", "C1", "U0", "D0", "C2"]), 0)
    sol.load(PyVRPSolution(data, [route]))
    op = RelocateAlternative(data)
    op.init(sol)
    pickup, _ = sol.shipments[0]
    assert_allclose(
        op.evaluate(pickup, sol.clients[2], CostEvaluator([], 0, 0)),
        (0, False),
    )


def test_relocate_shipment_between_routes(mixed_group):
    """
    Empty the source route and insert a complete alternative in the target.
    """
    matrix = np.array(mixed_group.distance_matrix(0))
    matrix[5, 6] = 1000
    data = mixed_group.replace(
        clients=[Client(1, required=False), Client(2), Client(0)],
        groups=[MutuallyExclusiveGroup([Activity("L0"), Activity("L1")])],
        vehicle_types=[VehicleType(2, fixed_cost=100)],
        distance_matrices=[matrix],
    )
    source = Route(data, map(Activity, ["L0", "U0"]), 0)
    target = Route(data, [1, 2], 0)
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [source, target]))
    op = RelocateAlternative(data)
    op.init(sol)
    pickup, _ = sol.shipments[0]
    V = sol.clients[1]
    assert_allclose(
        op.evaluate(pickup, V, CostEvaluator([], 0, 0)), (-110, True)
    )
    op.apply(pickup, V)
    assert_equal(str(sol.routes[0]), "")
    assert_equal(str(sol.routes[1]), "C1 L1 C2 U1")


def test_delivery_stays_in_pickup_trip(mixed_group):
    """
    A later delivery position cannot cross a reload depot.
    """
    matrix = np.array(mixed_group.distance_matrix(0))
    matrix[3, 4] = 100
    matrix[5, 6] = 100
    data = mixed_group.replace(
        clients=[*mixed_group.clients(), Client(2)],
        distance_matrices=[matrix],
    )
    sol = Solution(data)
    route = Route(data, map(Activity, ["C0", "C1", "D0", "C2"]), 0)
    sol.load(PyVRPSolution(data, [route]))
    op = RelocateAlternative(data)
    op.init(sol)
    move = op.evaluate(sol.clients[0], sol.clients[1], CostEvaluator([], 0, 0))
    assert_allclose(move, (0, False))


def test_skip_removed_shipment_nodes_and_deliveries(mixed_group):
    """
    V must survive removal of the shipment, and U must be its pickup.
    """
    sol = Solution(mixed_group)
    route = Route(mixed_group, map(Activity, ["L0", "C1", "U0"]), 0)
    sol.load(PyVRPSolution(mixed_group, [route]))
    op = RelocateAlternative(mixed_group)
    op.init(sol)
    pickup, delivery = sol.shipments[0]
    cost_eval = CostEvaluator([], 0, 0)
    assert_allclose(op.evaluate(pickup, delivery, cost_eval), (0, False))
    assert_allclose(
        op.evaluate(delivery, sol.clients[1], cost_eval), (0, False)
    )


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
    assert_allclose(move, (-81, True))

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
    assert_allclose(move, (-1690, True))

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
        groups=[
            MutuallyExclusiveGroup(
                [Activity("C0"), Activity("C1")], required=True
            )
        ],
    )

    # First scenario, with C0 and C2 on separate routes.
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0], [2]]))

    op = RelocateAlternative(data)
    op.init(sol)

    # Moving to the second route saves one fixed vehicle cost (100), and the
    # alternative collects 150 more units of prize.
    move = op.evaluate(sol.clients[0], sol.clients[2], CostEvaluator([], 0, 0))
    assert_allclose(move, (-250, True))

    # Second scenario, with C0 and C2 on the same route.
    sol = Solution(data)
    sol.load(PyVRPSolution(data, [[0, 2]]))
    op.init(sol)

    empty = sol.routes[1]
    assert_equal(empty.num_clients(), 0)

    # Moving to the empty second route adds one fixed vehicle cost (100),
    # while the cheaper alternative collects 150 more units of prize.
    move = op.evaluate(sol.clients[0], empty[0], CostEvaluator([], 0, 0))
    assert_allclose(move, (-50, True))


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
    assert_allclose(op.evaluate(route[1], route[2], cost_eval), (0, False))


def test_supports(
    gtsp,
    ok_small,
    ok_small_prizes,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that RelocateAlternative only supports data with mutually exclusive
    groups.
    """
    assert_(RelocateAlternative.supports(gtsp))
    assert_(RelocateAlternative.supports(ok_small_mutually_exclusive_groups))
    assert_(not RelocateAlternative.supports(ok_small))
    assert_(not RelocateAlternative.supports(ok_small_prizes))

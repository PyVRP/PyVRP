from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator
from pyvrp.search import RelocateShipment
from pyvrp.search._search import Route, Solution
from tests.helpers import make_search_route


def test_skip_same_route(small_shipments):
    """
    Tests that the operator skips evaluating moves for nodes in the same route.
    """
    sol = Solution(small_shipments)
    route = make_search_route(
        small_shipments,
        [*sol.shipments[0], *sol.shipments[1]],
    )

    op = RelocateShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], route[3], cost_eval), (0, False))


def test_skip_unassigned(small_shipments):
    """
    Tests that the operator skips evaluating moves for unassigned shipments.
    """
    sol = Solution(small_shipments)
    pickup, _ = sol.shipments[0]
    assert_(pickup.route is None)

    empty = Route(small_shipments, 0)

    # Evaluate relocating the unassigned shipment to just after the empty
    # route's starting depot.
    op = RelocateShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, empty[0], cost_eval), (0, False))


def test_skip_non_pickup(small_shipments):
    """
    Tests that the operator skips evaluating moves for non-pickup nodes U.
    """
    mixed = small_shipments.replace(clients=[Client(2, delivery=[0])])

    sol = Solution(mixed)
    route1 = make_search_route(mixed, [sol.clients[0]])  # C0
    route2 = make_search_route(mixed, [*sol.shipments[0]])  # L0 U0

    # Relocating the client C0 or the starting depot cannot be done by this
    # operator, so it should skip such moves.
    op = RelocateShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route1[1], route2[1], cost_eval), (0, False))
    assert_equal(op.evaluate(route1[0], route2[1], cost_eval), (0, False))


def test_fixed_cost(small_shipments):
    """
    Tests that the operator is aware of fixed vehicle cost, and accounts for it
    for moves that empty routes.
    """
    veh_type = small_shipments.vehicle_type(0).replace(fixed_cost=10_000)
    mixed = small_shipments.replace(
        clients=[Client(2, delivery=[0])],
        vehicle_types=[veh_type],
    )

    sol = Solution(mixed)
    route1 = make_search_route(mixed, [sol.clients[0]])  # C0
    route2 = make_search_route(mixed, [*sol.shipments[0]])  # L0 U0
    assert_equal(route1.distance() + route2.distance(), 13_789)

    # Evaluate relocating L0 U0 after C0. This is improving because C0 and L0
    # share a location, but also because it empties route2 and thus reduces the
    # fixed cost by 10_000.
    op = RelocateShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route2[1], route1[1], cost_eval), (-14_218, True))

    op.apply(route2[1], route1[1])
    route1.update()
    route2.update()

    assert_equal(route1.distance() + route2.distance(), 13_789 - 4_218)
    assert_equal(str(route1), "C0 L0 U0")
    assert_equal(str(route2), "")


def test_insert_not_adjacent(small_shipments):
    """
    Tests that the delivery node is inserted in the first improving place
    following pickup.
    """
    data = small_shipments

    sol = Solution(data)
    route1 = make_search_route(data, sol.shipments[2])
    route2 = make_search_route(data, [*sol.shipments[0], *sol.shipments[1]])
    assert_equal(route1.distance() + route2.distance(), 47_015)

    # Insert L2 just after U0, and delivery in the first improving place,
    # just after U1.
    op = RelocateShipment(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route1[1], route2[2], cost_eval), (-1_936, True))

    # Should insert U2 after U1, and L2 immediately after U0. L1 U1 is
    # in-between.
    op.apply(route1[1], route2[2])
    route1.update()
    route2.update()

    assert_equal(route1.distance() + route2.distance(), 47_015 - 1_936)
    assert_equal(str(route1), "")
    assert_equal(str(route2), "L0 U0 L2 L1 U1 U2")


def test_name(small_shipments):
    """
    Tests the operator's name.
    """
    op = RelocateShipment(small_shipments)
    assert_equal(op.name, "RelocateShipment")


def test_supports(ok_small, small_shipments, small_optional_shipments):
    """
    Tests that the operator supports instances with shipments.
    """
    # RelocateShipment supports any instance with shipments, optional or not.
    assert_(RelocateShipment.supports(small_shipments))
    assert_(RelocateShipment.supports(small_optional_shipments))

    # This instance has no shipments and is thus not supported.
    assert_(not RelocateShipment.supports(ok_small))

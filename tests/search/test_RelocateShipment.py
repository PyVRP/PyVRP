from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator, VehicleType
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


def test_fixed_cost_when_emptying_a_route(small_shipments):
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


def test_fixed_cost_relocating_into_empty_route(small_shipments):
    """
    Tests that the operator accounts for fixed cost when relocting to empty
    routes.
    """
    veh_type = small_shipments.vehicle_type(0).replace(fixed_cost=1_000)
    data = small_shipments.replace(vehicle_types=[veh_type])

    sol = Solution(data)
    activities = [*sol.shipments[1], *sol.shipments[0], *sol.shipments[2]]
    route1 = make_search_route(data, activities)
    route2 = Route(data, 0)
    assert_equal(route1.distance() + route2.distance(), 44_712)

    # Evaluate moving L0 U0 from route1 to route2, which is currently empty.
    # This reduces distance by 3_258, but also incurs route2's fixed cost of
    # 1_000. The resulting delta is thus -2_258.
    op = RelocateShipment(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route1[3], route2[0], cost_eval), (-2_258, True))

    op.apply(route1[3], route2[0])
    route1.update()
    route2.update()

    assert_equal(route1.distance() + route2.distance(), 44_712 - 3_258)
    assert_equal(str(route1), "L1 U1 L2 U2")
    assert_equal(str(route2), "L0 U0")


def test_relocate_non_adjacent_to_direct_sequence(small_shipments):
    """
    Tests relocating a shipment with nodes in-between pickup and delivery in
    its current route, to a direct sequence in the new route.
    """
    sol = Solution(small_shipments)
    pickup, delivery = sol.shipments[0]

    route1 = make_search_route(small_shipments, sol.shipments[1])
    activities2 = [pickup, *sol.shipments[2], delivery]
    route2 = make_search_route(small_shipments, activities2)
    assert_equal(route1.distance() + route2.distance(), 50_804)

    # Relocating U0 from route2 (L0 L2 U2 U0) to route1 (as L1 U1 L0 U0)
    # results in lower distance.
    op = RelocateShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route2[1], route1[2], cost_eval), (-1_275, True))

    op.apply(route2[1], route1[2])
    route1.update()
    route2.update()

    assert_equal(route1.distance() + route2.distance(), 50_804 - 1_275)
    assert_equal(str(route1), "L1 U1 L0 U0")
    assert_equal(str(route2), "L2 U2")


def test_relocate_non_adjacent_delivery(small_shipments):
    """
    Tests that the delivery node is inserted in the first improving place
    following pickup.
    """
    sol = Solution(small_shipments)
    route1 = make_search_route(small_shipments, sol.shipments[2])
    route2 = make_search_route(
        small_shipments,
        [*sol.shipments[0], *sol.shipments[1]],
    )

    assert_equal(route1.distance() + route2.distance(), 47_015)

    # Insert L2 just after U0, and delivery in the first improving place,
    # just after U1.
    op = RelocateShipment(small_shipments)
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
    Tests that the operator supports instances with shipments and multiple
    vehicles (not TSP).
    """
    # This instance has no shipments and is thus not supported.
    assert_(not RelocateShipment.supports(ok_small))

    # RelocateShipment supports any instance with shipments, optional or not.
    assert_(RelocateShipment.supports(small_shipments))
    assert_(RelocateShipment.supports(small_optional_shipments))

    # Now let's modify the instance to have just one vehicle. The operator does
    # not support TSP, so it does not support the modified instance.
    data = small_shipments.replace(vehicle_types=[VehicleType(capacity=[0])])
    assert_equal(data.num_vehicles, 1)
    assert_(not RelocateShipment.supports(data))

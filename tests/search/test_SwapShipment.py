from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator, VehicleType
from pyvrp.search import SwapShipment
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_swap_direct_sequence(small_shipments):
    """
    Tests the operator when swapping shipments that are both in direct
    sequence.
    """
    data = small_shipments

    sol = Solution(data)
    route1 = make_search_route(data, [*sol.shipments[0], *sol.shipments[2]])
    route2 = make_search_route(data, [*sol.shipments[1]])
    assert_equal(route1.distance() + route2.distance(), 48_644)

    # Swapping shipment 2 (on route1) and 1 (on route2) results in less
    # distance, and is thus an improving move.
    op = SwapShipment(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route1[3], route2[1], cost_eval), (-1_629, True))

    op.apply(route1[3], route2[1])
    route1.update()
    route2.update()

    # Verify the delta cost and end result.
    assert_equal(route1.distance() + route2.distance(), 48_644 - 1_629)
    assert_equal(str(route1), "L0 U0 L1 U1")
    assert_equal(str(route2), "L2 U2")


def test_swap_where_u_or_v_is_in_direct_sequence(small_shipments):
    """
    Tests the operator when swapping shipments where either U or V is in a
    direct sequence, but the other shipment is not.
    """
    data = small_shipments

    sol = Solution(data)
    pickup, delivery = sol.shipments[1]
    route1 = make_search_route(data, [pickup, *sol.shipments[2], delivery])
    route2 = make_search_route(data, [*sol.shipments[0]])
    assert_equal(route1.distance() + route2.distance(), 52_034)

    # We want to swap L1 and U1 (on route1, not in direct sequence) with L0
    # and U0 (on route2 in direct sequence). This move can be done in many
    # ways: either from L1 or L0, and against either the pickup or delivery
    # node of the other shipment. The resulting delta cost should be the same
    # in all cases.
    op = SwapShipment(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route1[1], route2[1], cost_eval), (-1_230, True))
    assert_equal(op.evaluate(route1[1], route2[2], cost_eval), (-1_230, True))
    assert_equal(op.evaluate(route2[1], route1[1], cost_eval), (-1_230, True))
    assert_equal(op.evaluate(route2[1], route1[4], cost_eval), (-1_230, True))

    op.apply(route2[1], route1[4])
    route1.update()
    route2.update()

    # Verify the delta cost and end result.
    assert_equal(route1.distance() + route2.distance(), 52_034 - 1_230)
    assert_equal(str(route1), "L0 L2 U2 U0")
    assert_equal(str(route2), "L1 U1")


def test_swap_general(small_shipments):
    """
    Tests the operator when swapping general shipments, where neither U nor V
    is in a direct sequence.
    """
    data = small_shipments

    sol = Solution(data)
    pickup2, delivery2 = sol.shipments[2]
    pickup3, delivery3 = sol.shipments[3]
    route1 = make_search_route(data, [pickup2, *sol.shipments[1], delivery2])
    route2 = make_search_route(data, [pickup3, *sol.shipments[0], delivery3])
    assert_equal(route1.distance() + route2.distance(), 75_351)

    # We want to swap L2 and U2 on route1 with L3 and U3 on route2. Both
    # shipments have nodes in between. This move can be done in many ways:
    # either from L2 or L3, and against the pickup or delivery node of the
    # other shipment. The resulting delta cost should be the same in all cases.
    op = SwapShipment(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route1[1], route2[1], cost_eval), (-14_219, True))
    assert_equal(op.evaluate(route1[1], route2[4], cost_eval), (-14_219, True))
    assert_equal(op.evaluate(route2[1], route1[1], cost_eval), (-14_219, True))
    assert_equal(op.evaluate(route2[1], route1[4], cost_eval), (-14_219, True))

    op.apply(route2[1], route1[4])
    route1.update()
    route2.update()

    # Verify the delta cost and end result.
    assert_equal(route1.distance() + route2.distance(), 75_351 - 14_219)
    assert_equal(str(route1), "L3 L1 U1 U3")
    assert_equal(str(route2), "L2 L0 U0 U2")


def test_skips_unassigned(small_shipments):
    """
    Tests that the operator skips moves when U is not assigned to a route.
    """
    sol = Solution(small_shipments)
    route = make_search_route(small_shipments, sol.shipments[1])
    assert_equal(str(route), "L1 U1")

    # Shipment 0 is not in any route, shipment 1 is in the route we just
    # constructed.
    pickup, _ = sol.shipments[0]
    _, delivery = sol.shipments[1]
    assert_(delivery.route is route and pickup.route is None)

    # This move cannot be evaluated because U is not in a route.
    op = SwapShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, delivery, cost_eval), (0, False))


def test_skips_same_route(small_shipments):
    """
    Tests that the operator cannot swap within the same route.
    """
    sol = Solution(small_shipments)
    route = make_search_route(
        small_shipments,
        [*sol.shipments[0], *sol.shipments[1]],
    )

    # Check that the shipments are indeed assigned to the same route.
    pickup, _ = sol.shipments[0]
    _, delivery = sol.shipments[1]
    assert_(pickup.route is delivery.route and pickup.route is route)

    # Test swapping the shipments. This should not work because the operator
    # cannot swap within the same route.
    op = SwapShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, delivery, cost_eval), (0, False))


def test_skips_clients_and_non_shipments(small_shipments):
    """
    Tests that the operator only works when both U and V are shipments, and U
    in particular is a pickup node.
    """
    data = small_shipments.replace(clients=[Client(2, delivery=[0])])
    sol = Solution(data)

    route1 = make_search_route(data, [sol.clients[0], *sol.shipments[1]])
    assert_equal(str(route1), "C0 L1 U1")

    route2 = make_search_route(data, [*sol.shipments[0]])
    assert_equal(str(route2), "L0 U0")

    client = sol.clients[0]
    pickup0, delivery0 = sol.shipments[0]
    pickup1, delivery1 = sol.shipments[1]

    op = SwapShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    # Swapping shipments 0 and 1 is an improving move, because C0 is at the
    # location of L0 and the move would thus reduce distance. It does not
    # matter whether V is a pickup or delivery node; the move is the same
    # either way.
    assert_equal(data.client(0).location, data.shipment(0).pickup.location)
    assert_equal(op.evaluate(pickup0, pickup1, cost_eval), (-2_927, True))
    assert_equal(op.evaluate(pickup0, delivery1, cost_eval), (-2_927, True))

    # But it does matter what U is: it *must* be a pickup node. U cannot be
    # a delivery or client node. And of course V must be a shipment.
    assert_equal(op.evaluate(delivery0, pickup1, cost_eval), (0, False))
    assert_equal(op.evaluate(client, pickup1, cost_eval), (0, False))
    assert_equal(op.evaluate(pickup0, client, cost_eval), (0, False))


def test_supports(ok_small, gtsp, small_shipments):
    """
    Tests that the operator supports instances with multiple routes and
    shipments.
    """
    # These instances have no shipments.
    assert_(not SwapShipment.supports(ok_small))
    assert_(not SwapShipment.supports(gtsp))

    # This instance has shipments and more than one vehicle, so the operator
    # supports it.
    assert_(SwapShipment.supports(small_shipments))

    # Now let's modify the instance to have just one vehicle. The operator does
    # not support TSP, so it does not support the modified instance.
    data = small_shipments.replace(vehicle_types=[VehicleType(capacity=[0])])
    assert_equal(data.num_vehicles, 1)
    assert_(not SwapShipment.supports(data))


def test_name(small_shipments):
    """
    Tests the operator's name.
    """
    op = SwapShipment(small_shipments)
    assert_equal(op.name, "SwapShipment")

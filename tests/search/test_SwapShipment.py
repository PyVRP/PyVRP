from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator, VehicleType
from pyvrp.search import SwapShipment
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_swap_direct_pairs():
    """
    Tests the operator when swapping shipments that are both in direct
    sequence.
    """
    pass  # TODO


def test_swap_where_u_is_a_direct_pair():
    """
    Tests the operator when swapping shipments where U is in a direct sequence,
    but V is not.
    """
    pass  # TODO


def test_swap_where_v_is_a_direct_pair():
    """
    Tests the operator when swapping shipments where V is in a direct sequence,
    but U is not.
    """
    pass  # TODO


def test_swap_general():
    """
    Tests the operator when swapping general shipments, where neither U nor V
    is in a direct sequence.
    """
    pass  # TODO


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

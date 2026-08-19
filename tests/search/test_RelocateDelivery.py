from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator
from pyvrp.search import RelocateDelivery
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_relocate_earlier_in_route(small_shipments):
    """
    Tests that the operator reinserts a delivery node earlier in the route if
    that is a better move.
    """
    sol = Solution(small_shipments)
    pickup, delivery = sol.shipments[1]
    route = make_search_route(
        small_shipments,
        [*sol.shipments[0], pickup, *sol.shipments[2], delivery],
    )

    # U1 is currently the last visit, but it is better for U2 to be the last
    # visit, by moving U1 to earlier in the route.
    assert_equal(route.distance(), 45_588)
    assert_equal(str(route), "L0 U0 L1 L2 U2 U1")

    op = RelocateDelivery(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[3], cost_eval), (-8_245, True))

    op.apply(route[3])
    route.update()

    assert_equal(route.distance(), 45_588 - 8_245)
    assert_equal(str(route), "L0 U0 L1 L2 U1 U2")


def test_relocate_just_before_depot(small_shipments):
    """
    Tests that the operator reinserts a delivery node just before the ending
    depot if that is a better move.
    """
    data = small_shipments.replace(clients=[Client(2, delivery=[0])])

    sol = Solution(data)
    client = sol.clients[0]
    pickup, delivery = sol.shipments[0]

    route = make_search_route(data, [pickup, delivery, client])
    assert_(client.route and pickup.route and delivery.route)
    assert_equal(route.distance(), 11_572)
    assert_equal(str(route), "L0 U0 C0")

    client_data = data.client(0)
    shipment_data = data.shipment(0)
    assert_equal(client_data.location, shipment_data.pickup.location)

    # U0 is currently just after L0, but a better place is just after C0. This
    # results in less distance because C0 is at L0's location.
    op = RelocateDelivery(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], cost_eval), (-2_001, True))

    op.apply(route[1])
    route.update()

    assert_equal(route.distance(), 11_572 - 2_001)
    assert_equal(str(route), "L0 C0 U0")


def test_reload_depot(small_shipments):
    """
    Tests that the operator does not reinsert a delivery node into a later
    trip.
    """
    clients = [Client(2, delivery=[0]), Client(1, delivery=[0])]
    veh_type = small_shipments.vehicle_type(0).replace(reload_depots=[0])
    data = small_shipments.replace(clients=clients, vehicle_types=[veh_type])

    sol = Solution(data)
    client1, client2 = sol.clients
    pick, deliv = sol.shipments[0]

    shipment = data.shipment(0)
    assert_equal(clients[1].location, shipment.delivery.location)

    # First trip serves L0 and U0, second trip C0 and C1. Without reloading,
    # U0 next to C1 would be improving since they share locations. But the
    # reload depot prevents that move.
    route = make_search_route(data, [pick, deliv, "D0", client1, client2])
    assert_(client1.route and client2.route and pick.route and deliv.route)
    assert_equal(str(route), "L0 U0 | C0 C1")

    op = RelocateDelivery(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], cost_eval), (0, False))


def test_relocate_skips_unassigned_nodes(small_shipments):
    """
    Tests that the operator cannot apply moves to unassigned shipments.
    """
    sol = Solution(small_shipments)
    pickup, _ = sol.shipments[0]
    assert_(not pickup.route)

    op = RelocateDelivery(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, cost_eval), (0, False))


def test_relocate_skips_non_pickup_nodes(small_shipments):
    """
    Tests that the operator skips client and delivery nodes. It only works for
    pickup nodes, despite relocating delivery nodes.
    """
    data = small_shipments.replace(clients=[Client(0, delivery=[0])])
    assert_(RelocateDelivery.supports(data))

    sol = Solution(data)
    client = sol.clients[0]
    pickup, delivery = sol.shipments[0]

    route = make_search_route(data, [client, pickup, delivery])
    assert_(client.route and pickup.route and delivery.route)
    assert_equal(str(route), "C0 L0 U0")

    op = RelocateDelivery(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(client, cost_eval), (0, False))
    assert_equal(op.evaluate(delivery, cost_eval), (0, False))


def test_supports(ok_small, small_shipments, small_optional_shipments):
    """
    Tests that the operator supports instances with shipments.
    """
    assert_(RelocateDelivery.supports(small_shipments))
    assert_(RelocateDelivery.supports(small_optional_shipments))

    # This instance has no shipments.
    assert_(not RelocateDelivery.supports(ok_small))


def test_name(small_shipments):
    """
    Tests the operator's name property.
    """
    op = RelocateDelivery(small_shipments)
    assert_equal(op.name, "RelocateDelivery")


def test_cannot_improve_singleton_route(small_shipments):
    """
    Tests that the operator cannot improve a singleton route.
    """
    sol = Solution(small_shipments)
    pickup, delivery = sol.shipments[0]

    route = make_search_route(small_shipments, [pickup, delivery])
    assert_(pickup.route and delivery.route)
    assert_equal(str(route), "L0 U0")

    op = RelocateDelivery(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, cost_eval), (0, False))

from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator
from pyvrp.constants import INT_MAX
from pyvrp.search import RelocatePickup
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_relocate_just_before_delivery(small_shipments):
    """
    Tests that the operator reinserts a pickup node just before the delivery
    node if that is the best move.
    """
    sol = Solution(small_shipments)
    pickup, delivery = sol.shipments[2]
    route = make_search_route(
        small_shipments,
        [pickup, *sol.shipments[0], *sol.shipments[1], delivery],
    )

    # L2 is now the first visit in the route, and U2 the last. But it is much
    # better to visit L2 U2 consecutively. Since U2 is fixed, that means L2
    # needs to move to the end of the route, just before U2.
    assert_equal(route.distance(), 43_414)
    assert_equal(str(route), "L2 L0 U0 L1 U1 U2")

    op = RelocatePickup(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], cost_eval), (-8_406, True))

    op.apply(route[1])
    route.update()

    assert_equal(route.distance(), 43_414 - 8_406)
    assert_equal(str(route), "L0 U0 L1 U1 L2 U2")


def test_relocate_just_after_depot(small_shipments):
    """
    Tests that the operator reinserts a pickup node just after the starting
    depot if that is the best move.
    """
    data = small_shipments.replace(clients=[Client(1, delivery=[0])])

    sol = Solution(data)
    client = sol.clients[0]
    pickup, delivery = sol.shipments[0]

    route = make_search_route(data, [client, pickup, delivery])
    assert_(client.route and pickup.route and delivery.route)
    assert_equal(route.distance(), 14_924)
    assert_equal(str(route), "C0 L0 U0")

    client_data = data.client(0)
    shipment_data = data.shipment(0)
    assert_equal(client_data.location, shipment_data.delivery.location)

    # L0 is currently just before U0, but a better place is to reinsert it just
    # before C0. That results in less distance, because C0 is at U0's location.
    op = RelocatePickup(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[2], cost_eval), (-5_353, True))

    op.apply(route[2])
    route.update()

    assert_equal(route.distance(), 14_924 - 5_353)
    assert_equal(str(route), "L0 C0 U0")


def test_reload_depot(small_shipments):
    """
    Tests that the operator does not reinsert a pickup node into an earlier
    trip.
    """
    clients = [Client(2, delivery=[0]), Client(1, delivery=[0])]
    veh_type = small_shipments.vehicle_type(0).replace(reload_depots=[0])
    data = small_shipments.replace(clients=clients, vehicle_types=[veh_type])

    sol = Solution(data)
    client1, client2 = sol.clients
    pick, deliv = sol.shipments[0]

    shipment = data.shipment(0)
    assert_equal(clients[0].location, shipment.pickup.location)

    # First trip serves C0 and C1, second trip L0 and U0. Without reloading,
    # L0 next to C0 would be improving since they share locations. But the
    # reload depot prevents that move.
    route = make_search_route(data, [client1, client2, "D0", pick, deliv])
    assert_(client1.route and client2.route and pick.route and deliv.route)
    assert_equal(str(route), "C0 C1 | L0 U0")

    op = RelocatePickup(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[4], cost_eval), (INT_MAX, False))


def test_relocate_skips_unassigned_nodes(small_shipments):
    """
    Tests that the operator cannot apply moves to unassigned pickup nodes.
    """
    sol = Solution(small_shipments)
    pickup, _ = sol.shipments[0]
    assert_(not pickup.route)

    op = RelocatePickup(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, cost_eval), (0, False))


def test_relocate_skips_non_pickup_nodes(small_shipments):
    """
    Tests that the operator skips client and delivery nodes. It only works for
    pickup nodes.
    """
    data = small_shipments.replace(clients=[Client(0, delivery=[0])])
    assert_(RelocatePickup.supports(data))

    sol = Solution(data)
    client = sol.clients[0]
    pickup, delivery = sol.shipments[0]

    route = make_search_route(data, [client, pickup, delivery])
    assert_(client.route and pickup.route and delivery.route)
    assert_equal(str(route), "C0 L0 U0")

    # The operator supports instances like these, but it cannot evaluate moves
    # for the client and delivery nodes.
    op = RelocatePickup(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(client, cost_eval), (0, False))
    assert_equal(op.evaluate(delivery, cost_eval), (0, False))


def test_supports(ok_small, small_shipments, small_optional_shipments):
    """
    Tests that the operator supports instances with shipments.
    """
    assert_(RelocatePickup.supports(small_shipments))
    assert_(RelocatePickup.supports(small_optional_shipments))

    # This instance has no shipments.
    assert_(not RelocatePickup.supports(ok_small))


def test_name(small_shipments):
    """
    Tests the operator's name property.
    """
    op = RelocatePickup(small_shipments)
    assert_equal(op.name, "RelocatePickup")


def test_cannot_improve_singleton_route(small_shipments):
    """
    Tests that the operator cannot improve a singleton route.
    """
    sol = Solution(small_shipments)
    pickup, delivery = sol.shipments[0]

    route = make_search_route(small_shipments, [pickup, delivery])
    assert_(pickup.route and delivery.route)
    assert_equal(str(route), "L0 U0")

    op = RelocatePickup(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, cost_eval), (INT_MAX, False))

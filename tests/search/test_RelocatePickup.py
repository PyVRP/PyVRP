import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator
from pyvrp.search import RelocatePickup
from pyvrp.search._search import Route, Solution

_INT_MAX = np.iinfo(np.int64).max


def test_relocate(small_shipments):
    """
    TODO
    """
    pass


def test_reload_depot(small_shipments):
    """
    TODO
    """
    pass


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

    route = Route(data, 0)
    route.append(client)
    route.append(pickup)
    route.append(delivery)
    route.update()

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

    route = Route(small_shipments, 0)
    route.append(pickup)
    route.append(delivery)
    route.update()

    assert_(pickup.route and delivery.route)
    assert_equal(str(route), "L0 U0")

    op = RelocatePickup(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, cost_eval), (_INT_MAX, False))

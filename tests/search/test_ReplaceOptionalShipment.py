from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import ReplaceOptionalShipment
from pyvrp.search._search import Solution
from tests.helpers import Route


def test_replace(small_optional_shipments):
    """
    Tests replacing an optional shipment with one that brings a higher prize
    and lower cost.
    """
    sol = Solution(small_optional_shipments)
    route = Route(small_optional_shipments, 0)
    route.append(sol.shipments[1][0])
    route.append(sol.shipments[1][1])
    route.update()

    assert_equal(route.distance(), 27_732)
    assert_equal(str(route), "L1 U1")

    op = ReplaceOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    # Evaluate replacing shipment 1 by shipment 0. This should be beneficial:
    # distance goes from 27_732 to 9_571, while the prize of 0 is 10_000 and of
    # 1 just 2_000. So the delta is -26_161. Note that it does not matter if we
    # evaluate the pickup or delivery node; either works fine.
    pickup, delivery = sol.shipments[0]
    assert_equal(op.evaluate(pickup, route[1], cost_eval), (-26_161, True))
    assert_equal(op.evaluate(pickup, route[2], cost_eval), (-26_161, True))
    assert_equal(op.evaluate(delivery, route[1], cost_eval), (-26_161, True))
    assert_equal(op.evaluate(delivery, route[2], cost_eval), (-26_161, True))

    op.apply(delivery, route[2])
    route.update()

    assert_equal(route.distance(), 9_571)
    assert_equal(str(route), "L0 U0")


def test_cannot_replace_required_shipment(small_shipments):
    """
    Tests that the operator cannot replace required shipments.
    """
    sol = Solution(small_shipments)
    route = Route(small_shipments, 0)
    route.append(sol.shipments[1][0])
    route.append(sol.shipments[1][1])
    route.update()

    op = ReplaceOptionalShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    assert_(small_shipments.shipment(0).required)
    assert_(small_shipments.shipment(1).required)

    # Each shipment is required. That means the shipment cannot be replaced,
    # since it would then leave the solution.
    pickup, delivery = sol.shipments[0]
    assert_equal(op.evaluate(pickup, route[1], cost_eval), (0, False))
    assert_equal(op.evaluate(delivery, route[1], cost_eval), (0, False))


def test_supports(ok_small, small_shipments, small_optional_shipments):
    """
    Tests supports().
    """
    # This instance only has clients, not shipments.
    assert_(not ReplaceOptionalShipment.supports(ok_small))

    # The operator only supports instances with *optional* shipments.
    assert_(not ReplaceOptionalShipment.supports(small_shipments))
    assert_(ReplaceOptionalShipment.supports(small_optional_shipments))

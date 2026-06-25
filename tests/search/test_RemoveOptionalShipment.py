from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import RemoveOptionalShipment
from pyvrp.search._search import Route, Solution


def test_remove(small_optional_shipments):
    """
    Tests removing a shipment that does not bring sufficient value.
    """
    sol = Solution(small_optional_shipments)
    route = Route(small_optional_shipments, 0)
    route.append(sol.shipments[1][0])
    route.append(sol.shipments[1][1])
    route.update()

    assert_equal(route.distance(), 27_732)
    assert_equal(str(route), "L1 U1")

    # The distance is 27_732, which will become zero after removal (the route
    # is empty then). The shipment's prize is 2_000, which we lose upon
    # removal. So the delta cost is 2_000 - 27_732 = -25_732. Note that it does
    # not matter whether we apply the operator to the pickup or delivery node
    # of the shipment.
    op = RemoveOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], cost_eval), (-25_732, True))
    assert_equal(op.evaluate(route[2], cost_eval), (-25_732, True))

    op.apply(route[2])
    route.update()

    assert_equal(route.distance(), 0)
    assert_equal(str(route), "")


def test_cannot_remove_required_shipment(small_shipments):
    """
    Tests that the operator cannot remove required shipments.
    """
    sol = Solution(small_shipments)
    route = Route(small_shipments, 0)
    route.append(sol.shipments[1][0])
    route.append(sol.shipments[1][1])
    route.update()

    op = RemoveOptionalShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    # The shipment is required and can thus not leave the solution.
    assert_(small_shipments.shipment(1).required)
    assert_equal(op.evaluate(route[1], cost_eval), (0, False))
    assert_equal(op.evaluate(route[2], cost_eval), (0, False))


def test_supports(ok_small, small_shipments, small_optional_shipments):
    """
    Tests supports().
    """
    # This instance only has clients, not shipments.
    assert_(not RemoveOptionalShipment.supports(ok_small))

    # The operator only supports instances with *optional* shipments.
    assert_(not RemoveOptionalShipment.supports(small_shipments))
    assert_(RemoveOptionalShipment.supports(small_optional_shipments))

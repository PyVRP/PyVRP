from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import ReplaceOptionalShipment
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_replace(small_optional_shipments):
    """
    Tests replacing an optional shipment with one that brings a higher prize
    and lower cost.
    """
    sol = Solution(small_optional_shipments)

    route = make_search_route(small_optional_shipments, sol.shipments[1])
    assert_equal(route.distance(), 27_732)
    assert_equal(str(route), "L1 U1")

    op = ReplaceOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    # Evaluate replacing shipment 1 by shipment 0. This should be beneficial:
    # distance goes from 27_732 to 9_571, while the prize of 0 is 10_000 and of
    # 1 just 2_000. So the delta is -26_161.
    pickup, _ = sol.shipments[0]
    assert_equal(op.evaluate(pickup, route[1], cost_eval), (-26_161, True))
    assert_equal(op.evaluate(pickup, route[2], cost_eval), (-26_161, True))

    op.apply(pickup, route[2])
    route.update()

    assert_equal(route.distance(), 9_571)
    assert_equal(str(route), "L0 U0")


def test_cannot_replace_required_shipment(small_shipments):
    """
    Tests that the operator cannot replace required shipments.
    """
    sol = Solution(small_shipments)
    route = make_search_route(small_shipments, sol.shipments[1])

    op = ReplaceOptionalShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    assert_(small_shipments.shipment(0).required)
    assert_(small_shipments.shipment(1).required)

    # Each shipment is required. That means the shipment cannot be replaced,
    # since it would then leave the solution.
    pickup, _ = sol.shipments[0]
    assert_equal(op.evaluate(pickup, route[1], cost_eval), (0, False))


def test_skips_deliveries(small_optional_shipments):
    """
    Tests that the operator skips delivery nodes because the local search only
    calls the operator with pickups.
    """
    sol = Solution(small_optional_shipments)
    pickup, delivery = sol.shipments[0]

    route = make_search_route(small_optional_shipments, sol.shipments[1])
    assert_equal(str(route), "L1 U1")

    op = ReplaceOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(delivery, route[1], cost_eval), (0, False))
    assert_equal(op.evaluate(pickup, route[1], cost_eval), (-26_161, True))


def test_supports(ok_small, small_shipments, small_optional_shipments):
    """
    Tests supports().
    """
    # This instance only has clients, not shipments.
    assert_(not ReplaceOptionalShipment.supports(ok_small))

    # The operator only supports instances with *optional* shipments.
    assert_(not ReplaceOptionalShipment.supports(small_shipments))
    assert_(ReplaceOptionalShipment.supports(small_optional_shipments))


def test_name(small_optional_shipments):
    """
    Tests the name property.
    """
    op = ReplaceOptionalShipment(small_optional_shipments)
    assert_equal(op.name, "ReplaceOptionalShipment")


def test_replace_non_adjacent(small_optional_shipments):
    """
    Tests replacing a shipment whose pickup and delivery nodes are not in
    direct sequence.
    """
    sol = Solution(small_optional_shipments)
    pickup1, delivery1 = sol.shipments[1]
    nodes = [pickup1, *sol.shipments[2], delivery1]

    route = make_search_route(small_optional_shipments, nodes)
    assert_equal(route.distance(), 42_463)
    assert_equal(str(route), "L1 L2 U2 U1")

    pickup, _ = sol.shipments[0]
    op = ReplaceOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, route[1], cost_eval), (-27_391, True))

    op.apply(pickup, route[1])
    route.update()

    # Cost delta is -27_391, but part of that is due to prizes: shipment 0
    # yields a prize of 10_000, while shipment 1 provides a prize of 2_000.
    # Thus, 10_000 - 2_000 = 8_000 of the delta is due to better prizes.
    assert_equal(route.distance(), 42_463 - 27_391 + 8_000)
    assert_equal(str(route), "L0 L2 U2 U0")

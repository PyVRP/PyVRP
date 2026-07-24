from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import RemoveOptionalShipment
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_remove(small_optional_shipments):
    """
    Tests removing a shipment that does not bring sufficient value.
    """
    sol = Solution(small_optional_shipments)

    route = make_search_route(small_optional_shipments, sol.shipments[1])
    assert_equal(route.distance(), 27_732)
    assert_equal(str(route), "L1 U1")

    # The distance is 27_732, which will become zero after removal (the route
    # is empty then). The shipment's prize is 2_000, which we lose upon
    # removal. So the delta cost is 2_000 - 27_732 = -25_732.
    op = RemoveOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], cost_eval), (-25_732, True))

    op.apply(route[1])
    route.update()

    assert_equal(route.distance(), 0)
    assert_equal(str(route), "")


def test_fixed_cost_empty_routes(small_optional_shipments):
    """
    Tests that the operator correctly removes fixed vehicle costs when removing
    the shipment leaves the route empty.
    """
    old_data = small_optional_shipments
    veh_type = old_data.vehicle_type(0).replace(fixed_cost=10_000)
    data = old_data.replace(vehicle_types=[veh_type])

    sol = Solution(data)
    route = make_search_route(data, sol.shipments[1])

    # See also the test above. The regular cost delta from distance and prizes
    # is -25_732, now increased by the fixed cost of 10_000 that we also lose
    # after removing the only shipment in the route.
    op = RemoveOptionalShipment(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], cost_eval), (-35_732, True))


def test_cannot_remove_required_shipment(small_shipments):
    """
    Tests that the operator cannot remove required shipments.
    """
    sol = Solution(small_shipments)
    route = make_search_route(small_shipments, sol.shipments[1])

    op = RemoveOptionalShipment(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    # The shipment is required and can thus not leave the solution.
    assert_(small_shipments.shipment(1).required)
    assert_equal(op.evaluate(route[1], cost_eval), (0, False))


def test_skips_deliveries(small_optional_shipments):
    """
    Tests that the operator skips delivery nodes because the local search only
    calls the operator with pickups.
    """
    op = RemoveOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    route = make_search_route(small_optional_shipments, ["L1", "U1"])
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


def test_name(small_optional_shipments):
    """
    Tests the name property.
    """
    op = RemoveOptionalShipment(small_optional_shipments)
    assert_equal(op.name, "RemoveOptionalShipment")


def test_remove_non_adjacent(small_optional_shipments):
    """
    Tests removing a shipment whose pickup and delivery nodes are not in
    direct sequence.
    """
    sol = Solution(small_optional_shipments)
    pickup1, delivery1 = sol.shipments[1]
    nodes = [pickup1, *sol.shipments[2], delivery1]

    route = make_search_route(small_optional_shipments, nodes)
    assert_equal(route.distance(), 42_463)
    assert_equal(str(route), "L1 L2 U2 U1")

    op = RemoveOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup1, cost_eval), (-24_305, True))

    op.apply(pickup1)
    route.update()

    # Cost delta is -24_305, but part of that is due to prizes: shipment 1
    # yields a prize of 2_000, which we have removed.
    assert_equal(route.distance(), 42_463 - 24_305 - 2_000)
    assert_equal(str(route), "L2 U2")

from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import InsertOptionalShipment
from pyvrp.search._search import Route, Solution
from tests.helpers import make_search_route


def test_insert_pickup_in_improving_place(small_optional_shipments):
    """
    Tests that U's pickup is inserted in the first improving place preceding
    delivery.
    """
    route = make_search_route(small_optional_shipments, ["L1", "U1"])
    assert_equal(route.num_shipments(), 1)
    assert_equal(route.distance(), 27_732)

    sol = Solution(small_optional_shipments)
    pickup, delivery = sol.shipments[0]
    assert_(pickup.is_pickup())
    assert_(delivery.is_delivery())

    # Insert delivery just before end depot, and pickup in an improving place
    # (first location). This results in a new distance of 32_401, but gets a
    # prize of 10_000, for a delta of -5_331.
    op = InsertOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(delivery, route[-2], cost_eval), (-5_331, True))

    # Should insert U0 after U1, and L0 immediately after the start depot.
    op.apply(delivery, route[-2])
    route.update()

    assert_equal(route.num_shipments(), 2)
    assert_equal(route.distance(), 32_401)
    assert_equal(small_optional_shipments.shipment(0).prize, 10_000)
    assert_equal(str(route), "L0 L1 U1 U0")


def test_insert_into_empty_route(small_optional_shipments):
    """
    Tests that shipments can be inserted into an empty route.
    """
    route = Route(small_optional_shipments, 0)
    assert_equal(route.num_shipments(), 0)

    sol = Solution(small_optional_shipments)
    pickup, delivery = sol.shipments[0]

    op = InsertOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    # We evaluate inserting the pickup and delivery nodes after the route's
    # start depot. The pickup is fine - for empty routes, we then evaluate
    # start -> pickup -> delivery -> end, a singleton route. For delivery we
    # cannot evaluate such a move, since start -> delivery is not valid (we
    # need a pickup first, but we want to insert delivery directly after the
    # second argument). Finally, the cost delta is
    #   distance - prize = 9_571 - 10_000 = -429.
    assert_equal(op.evaluate(pickup, route[0], cost_eval), (-429, True))
    assert_equal(op.evaluate(delivery, route[0], cost_eval), (0, False))


def test_insert_delivery_in_improving_place(small_optional_shipments):
    """
    Tests that U's delivery is inserted in the first improving place following
    pickup.
    """
    route = make_search_route(small_optional_shipments, ["L1", "U1"])
    assert_equal(route.num_shipments(), 1)
    assert_equal(route.distance(), 27_732)

    sol = Solution(small_optional_shipments)
    pickup, _ = sol.shipments[0]

    # Insert pickup just after start depot, and delivery immediately after.
    # This results in a new distance of 30_857, but gets a prize of 10_000, for
    # a delta of -6_875.
    op = InsertOptionalShipment(small_optional_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, route[0], cost_eval), (-6875, True))

    # Should insert L0 U0 immediately after the start depot.
    op.apply(pickup, route[0])
    route.update()

    assert_equal(route.num_shipments(), 2)
    assert_equal(route.distance(), 30_857)
    assert_equal(small_optional_shipments.shipment(0).prize, 10_000)
    assert_equal(str(route), "L0 U0 L1 U1")


def test_skip_if_shipment_already_in_route(small_shipments):
    """
    Tests that the operator skips evaluation if U is already in a route (or if
    V is not).
    """
    route = make_search_route(small_shipments, ["L0", "U0", "L1", "U1"], 0)

    op = InsertOptionalShipment(small_shipments)
    cost_eval = CostEvaluator([10], 0, 0)

    assert_(route[1].route is not None)
    assert_equal(op.evaluate(route[1], route[3], cost_eval), (0, False))


def test_supports(small_optional_shipments, small_shipments):
    """
    Tests that the operator supports instances with optional shipments.
    """
    assert_(InsertOptionalShipment.supports(small_optional_shipments))
    assert_(not InsertOptionalShipment.supports(small_shipments))

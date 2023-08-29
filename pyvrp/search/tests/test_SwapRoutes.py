from typing import List

import pytest
from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import CostEvaluator, VehicleType
from pyvrp.search import SwapRoutes
from pyvrp.search._search import Node, Route
from pyvrp.tests.helpers import make_heterogeneous, read


@pytest.mark.parametrize(
    ("visits1", "visits2"),
    [
        ([], []),  # both empty
        ([1], []),  # first non-empty, second empty
        ([], [1]),  # first empty, second non-empty
        ([1], [2, 3]),  # both non-empty but unequal length
        ([2, 3], [1]),  # both non-empty but unequal length (flipped)
        ([2, 3], [1, 4]),  # both non-empty equal length
    ],
)
def test_apply(visits1: List[int], visits2: List[int]):
    """
    Tests that applying SwapRoutes to two different routes indeed exchanges
    the visits.
    """
    data = read("data/OkSmall.txt")

    route1 = Route(data, idx=0, vehicle_type=0)
    for loc in visits1:
        route1.append(Node(loc=loc))

    route2 = Route(data, idx=1, vehicle_type=0)
    for loc in visits2:
        route2.append(Node(loc=loc))

    route1.update()
    route2.update()

    # Before calling apply, route1 visits the clients in visits1, and route2
    # visits the clients in visits2.
    assert_equal(visits1, [node.client for node in route1])
    assert_equal(visits2, [node.client for node in route2])

    op = SwapRoutes(data)
    op.apply(route1, route2)

    # But after apply, the visits are now swapped.
    assert_equal(visits2, [node.client for node in route1])
    assert_equal(visits1, [node.client for node in route2])


def test_evaluate_same_vehicle_type():
    """
    Tests that evaluate() returns 0 in case the same vehicle types are used,
    since in that case swapping cannot result in cost savings.
    """
    data = read("data/OkSmall.txt")

    route1 = Route(data, idx=0, vehicle_type=0)
    route2 = Route(data, idx=1, vehicle_type=0)
    assert_equal(route1.vehicle_type, route2.vehicle_type)

    route1.append(Node(loc=1))
    route2.append(Node(loc=2))

    route1.update()
    route2.update()

    op = SwapRoutes(data)
    cost_eval = CostEvaluator(1, 1)
    assert_allclose(op.evaluate(route1, route2, cost_eval), 0)


def test_evaluate_empty_routes():
    """
    Tests that evaluate() returns 0 when one or both of the routes are empty.
    """
    # First vehicle type is the same as default OkSmall. The second one has
    # 10 fixed cost, instead of 0.
    vehicle_types = [VehicleType(10, 3, 0), VehicleType(10, 3, 10)]
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, vehicle_types)

    route1 = Route(data, idx=0, vehicle_type=0)
    route2 = Route(data, idx=1, vehicle_type=1)
    route3 = Route(data, idx=2, vehicle_type=0)

    route1.append(Node(loc=1))

    route1.update()
    route2.update()

    op = SwapRoutes(data)
    cost_eval = CostEvaluator(1, 1)

    # Vehicle types are no longer the same, but one of the routes is empty.
    # That situation is not currently handled.
    assert_(route1.vehicle_type != route2.vehicle_type)
    assert_allclose(op.evaluate(route1, route2, cost_eval), 0)
    assert_allclose(op.evaluate(route2, route1, cost_eval), 0)

    # Both routes are empty, but of different vehicle type as well.
    assert_equal(len(route2), len(route3))
    assert_allclose(op.evaluate(route3, route2, cost_eval), 0)

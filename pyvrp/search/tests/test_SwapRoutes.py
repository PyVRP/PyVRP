from typing import List

import pytest
from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import CostEvaluator, VehicleType
from pyvrp.search import SwapRoutes
from pyvrp.search._search import Node, Route
from pyvrp.tests.helpers import make_heterogeneous


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
def test_apply(ok_small, visits1: List[int], visits2: List[int]):
    """
    Tests that applying SwapRoutes to two different routes indeed exchanges
    the visits.
    """
    route1 = Route(ok_small, idx=0, vehicle_type=0)
    for loc in visits1:
        route1.append(Node(loc=loc))

    route2 = Route(ok_small, idx=1, vehicle_type=0)
    for loc in visits2:
        route2.append(Node(loc=loc))

    route1.update()
    route2.update()

    # Before calling apply, route1 visits the clients in visits1, and route2
    # visits the clients in visits2.
    assert_equal(visits1, [node.client for node in route1])
    assert_equal(visits2, [node.client for node in route2])

    op = SwapRoutes(ok_small)
    op.apply(route1, route2)

    # But after apply, the visits are now swapped.
    assert_equal(visits2, [node.client for node in route1])
    assert_equal(visits1, [node.client for node in route2])


def test_evaluate_same_vehicle_type(ok_small):
    """
    Tests that evaluate() returns 0 in case the same vehicle types are used,
    since in that case swapping cannot result in cost savings.
    """
    route1 = Route(ok_small, idx=0, vehicle_type=0)
    route2 = Route(ok_small, idx=1, vehicle_type=0)
    assert_equal(route1.vehicle_type, route2.vehicle_type)

    route1.append(Node(loc=1))
    route2.append(Node(loc=2))

    route1.update()
    route2.update()

    op = SwapRoutes(ok_small)
    cost_eval = CostEvaluator(1, 1)
    assert_allclose(op.evaluate(route1, route2, cost_eval), 0)


def test_evaluate_empty_routes(ok_small):
    """
    Tests that evaluate() returns 0 when one or both of the routes are empty.
    """
    data = make_heterogeneous(
        ok_small, [VehicleType(10, 3), VehicleType(10, 3)]
    )

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


def test_evaluate_capacity_differences(ok_small):
    """
    Tests that changes in vehicle capacity violations are evaluated correctly.
    """
    data = make_heterogeneous(
        ok_small, [VehicleType(10, 1), VehicleType(20, 1)]
    )

    route1 = Route(data, idx=0, vehicle_type=0)
    for loc in [1, 2, 4]:
        route1.append(Node(loc=loc))

    route2 = Route(data, idx=0, vehicle_type=1)
    route2.append(Node(loc=3))

    route1.update()
    route2.update()

    # route1 has vehicle type 0, which has capacity 10. So there is excess load
    # since its client demand sums to 15.
    assert_(route1.has_excess_load())
    assert_allclose(route1.load(), 15)

    # route2, on the other hand, has capacity 20 and a load of only 3.
    assert_(not route2.has_excess_load())
    assert_allclose(route2.load(), 3)

    op = SwapRoutes(data)
    cost_eval = CostEvaluator(40, 1)

    # Swapping the route plans should alleviate the excess load, since the load
    # of 15 on route1 is below route2's capacity, and similarly for route2's
    # load and route1's capacity. Since we price unit load violations at 40,
    # this should result in a delta cost of -200. The operator is symmetric,
    # so evaluate(route1, route2) and evaluate(route2, route1) are the same.
    assert_allclose(op.evaluate(route1, route2, cost_eval), -200)
    assert_allclose(op.evaluate(route2, route1, cost_eval), -200)


def test_evaluate_shift_time_window_differences(ok_small):
    """
    Tests that SwapRoutes correctly evaluates changes in time warp due to
    different shift time windows.
    """
    data = make_heterogeneous(
        ok_small,
        [
            VehicleType(10, 1, tw_early=10_000, tw_late=15_000),
            VehicleType(10, 1, tw_early=15_000, tw_late=20_000),
        ],
    )

    route1 = Route(data, idx=0, vehicle_type=0)
    for loc in [1, 4]:  # depot -> 1 -> 4 -> depot
        route1.append(Node(loc=loc))
    route1.update()

    route2 = Route(data, idx=0, vehicle_type=1)
    for loc in [3, 2]:  # depot -> 3 -> 2 -> depot
        route2.append(Node(loc=loc))
    route2.update()

    # Without shift time windows, both routes are feasible, and there is slack
    # on either route: the first route can start between [14'056, 16'003], and
    # the second between [9'002, 13'369]. Neither route can complete its visits
    # within the shift time window of its assigned vehicle type. However, the
    # other type has a shift duration that is much better aligned with its
    # route. Thus, we should have that swapping the vehicle types results in
    # a lower cost, due to decreased time warp on the routes.
    op = SwapRoutes(data)
    cost_eval = CostEvaluator(1, 1)
    assert_(op.evaluate(route1, route2, cost_eval) < 0)

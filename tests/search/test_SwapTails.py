import numpy as np
from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import (
    Client,
    CostEvaluator,
    Depot,
    ProblemData,
    RandomNumberGenerator,
    Solution,
    VehicleType,
)
from pyvrp import Route as SolRoute
from pyvrp.search import LocalSearch, SwapTails
from pyvrp.search._search import Node, Route


@mark.parametrize(
    "vehicle_types",
    [
        [VehicleType(capacity=[10]), VehicleType(capacity=[10])],
        [VehicleType(capacity=[8]), VehicleType(capacity=[10])],
        [VehicleType(capacity=[10]), VehicleType(capacity=[8])],
        [VehicleType(capacity=[9]), VehicleType(capacity=[9])],
        [VehicleType(capacity=[8]), VehicleType(capacity=[8])],
    ],
)
def test_OkSmall_multiple_vehicle_types(
    ok_small, vehicle_types: list[VehicleType]
):
    """
    This test evaluates a move that is improving for some of the vehicle types,
    and not for others. Because the granular neighbourhood is very constrained,
    only a single move can be applied. When it's better to do so, it should
    have been applied, and when it's not better, it should not.
    """
    data = ok_small.replace(vehicle_types=vehicle_types)

    cost_evaluator = CostEvaluator(10_000, 6, 0)  # large load penalty
    rng = RandomNumberGenerator(seed=42)

    neighbours: list[list[int]] = [[], [2], [], [], []]  # only 1 -> 2
    ls = LocalSearch(data, rng, neighbours)
    ls.add_node_operator(SwapTails(data))

    routes1 = [SolRoute(data, [1, 3], 0), SolRoute(data, [2, 4], 1)]
    sol1 = Solution(data, routes1)

    routes2 = [SolRoute(data, [1, 4], 0), SolRoute(data, [2, 3], 1)]
    sol2 = Solution(data, routes2)

    cost1 = cost_evaluator.penalised_cost(sol1)
    cost2 = cost_evaluator.penalised_cost(sol2)
    assert_(not np.allclose(cost1, cost2))

    # Using the local search, the result should not get worse.
    improved_sol1 = ls.search(sol1, cost_evaluator)
    improved_sol2 = ls.search(sol2, cost_evaluator)

    expected_sol = sol1 if cost1 < cost2 else sol2
    assert_equal(improved_sol1, expected_sol)
    assert_equal(improved_sol2, expected_sol)


def test_move_involving_empty_routes():
    """
    This test checks that a SwapTails move that turns an empty route into a
    non-empty route (or vice-versa) is correctly evaluated.
    """
    data = ProblemData(
        clients=[Client(x=1, y=1), Client(x=1, y=0)],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[
            VehicleType(fixed_cost=10),
            VehicleType(fixed_cost=100),
        ],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    route1 = Route(data, idx=0, vehicle_type=0)
    route2 = Route(data, idx=1, vehicle_type=1)

    for loc in [1, 2]:
        route1.append(Node(loc=loc))

    route1.update()  # depot -> 1 -> 2 -> depot
    route2.update()  # depot -> depot

    op = SwapTails(data)
    cost_eval = CostEvaluator(0, 0, 0)

    # This move does not change the route structure, so the delta cost is 0.
    assert_equal(op.evaluate(route1[2], route2[0], cost_eval), 0)

    # This move creates routes (depot -> 1 -> depot) and (depot -> 2 -> depot),
    # making route 2 non-empty and thus incurring its fixed cost of 100.
    assert_equal(op.evaluate(route1[1], route2[0], cost_eval), 100)

    # This move creates routes (depot -> depot) and (depot -> 1 -> 2 -> depot),
    # making route 1 empty, while making route 2 non-empty. The total fixed
    # cost incurred is thus -10 + 100 = 90.
    assert_equal(op.evaluate(route1[0], route2[0], cost_eval), 90)

    # Now we reverse the visits of route 1 and 2, so that we can hit the cases
    # where route 1 is empty.
    route1.clear()

    for loc in [1, 2]:
        route2.append(Node(loc=loc))

    route1.update()  # depot -> depot
    route2.update()  # depot -> 1 -> 2 -> depot

    # This move does not change the route structure, so the delta cost is 0.
    assert_equal(op.evaluate(route1[0], route2[2], cost_eval), 0)

    # This move creates routes (depot -> 2 -> depot) and (depot -> 1 -> depot),
    # making route 1 non-empty and thus incurring its fixed cost of 10.
    assert_equal(op.evaluate(route1[0], route2[1], cost_eval), 10)

    # This move creates routes (depot -> 1 -> 2 -> depot) and (depot -> depot),
    # making route 1 non-empty, while making route 2 empty. The total fixed
    # cost incurred is thus 10 - 100 = -90.
    assert_equal(op.evaluate(route1[0], route2[0], cost_eval), -90)


def test_move_involving_multiple_depots():
    """
    Tests that SwapTails correctly evaluates distance changes due to different
    start and end depots of different vehicle types.
    """
    # Locations with indices 0 and 1 are depots, with 2 and 3 are clients.
    data = ProblemData(
        clients=[Client(x=1, y=1), Client(x=4, y=4)],
        depots=[Depot(x=0, y=0), Depot(x=5, y=5)],
        vehicle_types=[
            VehicleType(start_depot=0, end_depot=0),
            VehicleType(start_depot=1, end_depot=1),
        ],
        distance_matrices=[
            [
                [0, 10, 2, 8],
                [10, 0, 8, 2],
                [2, 8, 0, 6],
                [8, 2, 6, 0],
            ]
        ],
        duration_matrices=[np.zeros((4, 4), dtype=int)],
    )

    # First route is 0 -> 3 -> 0.
    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(Node(loc=3))
    route1.update()

    # Second route is 1 -> 2 -> 1.
    route2 = Route(data, idx=1, vehicle_type=1)
    route2.append(Node(loc=2))
    route2.update()

    assert_equal(route1.distance(), 16)
    assert_equal(route2.distance(), 16)

    op = SwapTails(data)
    cost_eval = CostEvaluator(1, 1, 0)

    assert_equal(op.evaluate(route1[1], route2[1], cost_eval), 0)  # no-op

    # First would be 0 -> 3 -> 2 -> 0, second 1 -> 1. Distance on route2 would
    # be zero, and on route1 16. Thus delta cost is -16.
    assert_equal(op.evaluate(route1[1], route2[0], cost_eval), -16)

    # First would be 0 -> 0, second 1 -> 2 -> 3 -> 1. Distance on route1 would
    # be zero, and on route2 16. Thus delta cost is -16.
    assert_equal(op.evaluate(route1[0], route2[1], cost_eval), -16)


def test_move_with_different_profiles(ok_small_two_profiles):
    """
    Tests that SwapTails correctly evaluates moves between routes with
    different profiles.
    """
    data = ok_small_two_profiles
    dist1, dist2 = data.distance_matrices()

    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(Node(loc=3))
    route1.update()

    route2 = Route(data, idx=1, vehicle_type=1)
    route2.append(Node(loc=2))
    route2.update()

    op = SwapTails(data)
    cost_eval = CostEvaluator(0, 0, 0)  # all zero so no costs from penalties

    # First route has profile 0, and its distance is thus computed using the
    # first distance matrix.
    assert_equal(route1.profile(), 0)
    assert_equal(route1.distance(), dist1[0, 3] + dist1[3, 0])

    # Second route has profile 1, and its distance is thus computed using the
    # second distance matrix.
    assert_equal(route2.profile(), 1)
    assert_equal(route2.distance(), dist2[0, 2] + dist2[2, 0])

    # This move evaluates the setting where the second route would be empty,
    # and the first becomes 0 -> 3 -> 2 -> 0.
    delta = dist1[3, 2] + dist1[2, 0] - dist1[3, 0] - route2.distance()
    assert_equal(op.evaluate(route1[1], route2[0], cost_eval), delta)

    # This move evaluates the setting where the first route would be empty, and
    # the second becomes 0 -> 2 -> 3 -> 0.
    delta = dist2[2, 3] + dist2[3, 0] - dist2[2, 0] - route1.distance()
    assert_equal(op.evaluate(route1[0], route2[1], cost_eval), delta)


def test_swapping_routes_with_multiple_trips():
    """
    Tests that SwapTails correctly evaluates and applies a move where two
    routes are swapped both consisting of multiple trips.
    """
    data = ProblemData(
        clients=[
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[
            VehicleType(capacity=[5], max_trips=2),
            VehicleType(capacity=[10], max_trips=2),
        ],
        distance_matrices=[np.zeros((7, 7), dtype=int)],
        duration_matrices=[np.zeros((7, 7), dtype=int)],
    )

    # First route is 0 -> 3 -> 4 -> 0 -> 0 -> 5 -> 6 -> 0.
    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(Node(loc=3))
    route1.append(Node(loc=4))
    route1.add_trip()
    route1.append(Node(loc=5))
    route1.append(Node(loc=6))
    route1.update()

    # Second route is 0 -> 1 -> 0 -> 0 -> 2 -> 0.
    route2 = Route(data, idx=1, vehicle_type=1)
    route2.append(Node(loc=1))
    route2.add_trip()
    route2.append(Node(loc=2))
    route2.update()

    assert_(route1.has_excess_load())
    assert_equal(route1.excess_load(), [10])  # Both trips have 5 excess load
    assert_(not route2.has_excess_load())
    assert_equal(route2.excess_load(), [0])

    op = SwapTails(data)
    cost_eval = CostEvaluator(1, 0, 0)

    # Swap routes. This should fix the excess load on route 1 without resulting
    # in any excess load on route 2. Thus delta cost is -10.
    assert_equal(op.evaluate(route1[0], route2[0], cost_eval), -10)
    op.apply(route1[0], route2[0])

    route1.update()
    route2.update()

    assert_equal(route1.excess_load(), [0])
    assert_equal(route1.num_trips(), 2)
    assert_equal(route1.num_clients(), 2)
    assert_equal(len(route1), 6)

    assert_equal(route2.excess_load(), [0])
    assert_equal(route2.num_trips(), 2)
    assert_equal(route2.num_clients(), 4)
    assert_equal(len(route2), 8)


def test_move_involving_multiple_trips():
    """
    Tests that SwapTails correctly evaluates moves concerning multiple trips.
    """
    data = ProblemData(
        clients=[
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[
            VehicleType(capacity=[5], max_trips=2),
            VehicleType(capacity=[10], max_trips=2),
        ],
        distance_matrices=[np.zeros((7, 7), dtype=int)],
        duration_matrices=[np.zeros((7, 7), dtype=int)],
    )

    # First route is 0 -> 3 -> 4 -> 0 -> 0 -> 5 -> 6 -> 0.
    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(Node(loc=3))
    route1.append(Node(loc=4))
    route1.add_trip()
    route1.append(Node(loc=5))
    route1.append(Node(loc=6))
    route1.update()

    # Second route is 0 -> 1 -> 0 -> 0 -> 2 -> 0.
    route2 = Route(data, idx=1, vehicle_type=1)
    route2.append(Node(loc=1))
    route2.add_trip()
    route2.append(Node(loc=2))
    route2.update()

    assert_(route1.has_excess_load())
    assert_equal(route1.excess_load(), [10])  # Both trips have 5 excess load
    assert_(not route2.has_excess_load())
    assert_equal(route2.excess_load(), [0])

    op = SwapTails(data)
    cost_eval = CostEvaluator(1, 0, 0)

    # The first route becomes: 0 -> 3 -> 1 -> 0 -> 0 -> 2 -> 0. The second
    # route becomes: 0 -> 4 -> 0 -> 0 -> 5 -> 6 -> 0. There is a reduction of 5
    # in excess load on the first route, thus delta cost is -5.
    assert_equal(op.evaluate(route1[1], route2[0], cost_eval), -5)
    op.apply(route1[1], route2[0])

    route1.update()
    route2.update()

    assert_equal(route1.excess_load(), [5])
    assert_equal(route1.num_trips(), 2)
    assert_equal(route1.num_clients(), 3)
    assert_equal(len(route1), 7)

    assert_equal(route2.excess_load(), [0])
    assert_equal(route2.num_trips(), 2)
    assert_equal(route2.num_clients(), 3)
    assert_equal(len(route2), 7)


def test_move_different_number_of_trips_swapped():
    """
    Tests that SwapTails correctly evaluates moves where the tail of route U
    has a different number of trips than the tail of route V.
    """
    data = ProblemData(
        clients=[
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[
            VehicleType(capacity=[5], max_trips=4),
            VehicleType(capacity=[10], max_trips=4),
        ],
        distance_matrices=[np.zeros((7, 7), dtype=int)],
        duration_matrices=[np.zeros((7, 7), dtype=int)],
    )

    # First route is 0 -> 3 -> 0 -> 0 -> 4 -> 0 -> 0 -> 5 -> 6 -> 0.
    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(Node(loc=3))
    route1.add_trip()
    route1.append(Node(loc=4))
    route1.add_trip()
    route1.append(Node(loc=5))
    route1.append(Node(loc=6))

    route1.update()

    # Second route is 0 -> 1 -> 0 -> 0 -> 2 -> 0.
    route2 = Route(data, idx=1, vehicle_type=1)
    route2.append(Node(loc=1))
    route2.add_trip()
    route2.append(Node(loc=2))
    route2.update()

    assert_(route1.has_excess_load())
    assert_equal(route1.excess_load(), [5])  # Last trip has 5 excess load
    assert_(not route2.has_excess_load())
    assert_equal(route2.excess_load(), [0])

    op = SwapTails(data)
    cost_eval = CostEvaluator(1, 0, 0)

    # The first route becomes: 0 -> 2 -> 0. The second route becomes:
    # 0 -> 1 -> 0 -> 0 -> 3 -> 0 -> 0 -> 4 -> 0 -> 0 -> 5 -> 6 -> 0.
    # There is a reduction of 5 in excess load on the first route without
    # resulting in excess in the second route. Thus delta cost is -5.
    assert_equal(op.evaluate(route1[0], route2[3], cost_eval), -5)
    op.apply(route1[0], route2[3])

    route1.update()
    route2.update()

    assert_equal(route1.excess_load(), [0])
    assert_equal(route1.num_trips(), 1)
    assert_equal(route1.num_clients(), 1)
    assert_equal(len(route1), 3)

    assert_equal(route2.excess_load(), [0])
    assert_equal(route2.num_trips(), 4)
    assert_equal(route2.num_clients(), 5)
    assert_equal(len(route2), 13)


def test_move_losing_trip():
    """
    Tests that SwapTails correctly evaluates moves when swapping the first
    parts of the tails results in an empty trip.
    """
    data = ProblemData(
        clients=[
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[
            VehicleType(capacity=[5], max_trips=2),
            VehicleType(capacity=[10], max_trips=2),
        ],
        distance_matrices=[np.zeros((7, 7), dtype=int)],
        duration_matrices=[np.zeros((7, 7), dtype=int)],
    )

    # First route is 0 -> 3 -> 4 -> 0 -> 0 -> 5 -> 6 -> 0.
    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(Node(loc=3))
    route1.append(Node(loc=4))
    route1.add_trip()
    route1.append(Node(loc=5))
    route1.append(Node(loc=6))
    route1.update()

    # Second route is 0 -> 1 -> 0 -> 0 -> 2 -> 0.
    route2 = Route(data, idx=1, vehicle_type=1)
    route2.append(Node(loc=1))
    route2.add_trip()
    route2.append(Node(loc=2))
    route2.update()

    assert_(route1.has_excess_load())
    assert_equal(route1.excess_load(), [10])  # Both trips have 5 excess load
    assert_(not route2.has_excess_load())
    assert_equal(route2.excess_load(), [0])

    op = SwapTails(data)
    cost_eval = CostEvaluator(1, 0, 0)

    # The first route becomes: 0 -> 3 -> 4 -> 1 -> 0 -> 0 -> 2 -> 0. The second
    # route becomes: 0 -> 5 -> 6 -> 0. Note that a trip became empty during the
    # swap and is removed. The first trip of the first route has an excess load
    # of 10, where the second trip has no excess load. Thus delta cost is 0.
    assert_equal(op.evaluate(route1[2], route2[0], cost_eval), 0)

    op.apply(route1[2], route2[0])

    route1.update()
    route2.update()

    assert_equal(route1.excess_load(), [10])
    assert_equal(route1.num_trips(), 2)
    assert_equal(route1.num_clients(), 4)
    assert_equal(len(route1), 8)

    assert_equal(route2.excess_load(), [0])
    assert_equal(route2.num_trips(), 1)
    assert_equal(route2.num_clients(), 2)
    assert_equal(len(route2), 4)


def test_move_not_exceeding_max_trips():
    """
    Tests that SwapTails correctly evaluates moves where the maximum number of
    trips is not exceeded, because empty trips are removed and do not count.
    """
    data = ProblemData(
        clients=[
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[
            VehicleType(capacity=[5], max_trips=2),
            VehicleType(capacity=[15], max_trips=1),
        ],
        distance_matrices=[np.zeros((7, 7), dtype=int)],
        duration_matrices=[np.zeros((7, 7), dtype=int)],
    )

    # First route is 0 -> 3 -> 4 -> 0 -> 0 -> 5 -> 2 -> 6 -> 0.
    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(Node(loc=3))
    route1.append(Node(loc=4))
    route1.add_trip()
    route1.append(Node(loc=5))
    route1.append(Node(loc=2))
    route1.append(Node(loc=6))
    route1.update()

    # Second route is 0 -> 1 -> 0.
    route2 = Route(data, idx=1, vehicle_type=1)
    route2.append(Node(loc=1))
    route2.update()

    assert_(route1.has_excess_load())
    assert_equal(route1.excess_load(), [15])
    assert_(not route2.has_excess_load())
    assert_equal(route2.excess_load(), [0])

    op = SwapTails(data)
    cost_eval = CostEvaluator(1, 0, 0)

    # The first route becomes: 0 -> 3 -> 4 -> 1 -> 0. The second route becomes:
    # 0 -> 5 -> 2 -> 6 -> 0. Note that a trip is lost during the swap.
    # The first trip of the first route has an excess load of 10, where the
    # second trip has no excess load. Thus delta cost is -5.
    assert_equal(op.evaluate(route1[2], route2[0], cost_eval), -5)
    op.apply(route1[2], route2[0])

    route1.update()
    route2.update()

    assert_equal(route1.excess_load(), [10])
    assert_equal(route1.num_trips(), 1)
    assert_equal(route1.num_clients(), 3)
    assert_equal(len(route1), 5)

    assert_equal(route2.excess_load(), [0])
    assert_equal(route2.num_trips(), 1)
    assert_equal(route2.num_clients(), 3)
    assert_equal(len(route2), 5)


def test_move_exceeding_max_trips():
    """
    Tests that SwapTails correctly evaluates moves where the maximum number of
    trips is exceeded.
    """
    data = ProblemData(
        clients=[
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[
            VehicleType(capacity=[5], max_trips=2),
            VehicleType(capacity=[15], max_trips=1),
        ],
        distance_matrices=[np.zeros((7, 7), dtype=int)],
        duration_matrices=[np.zeros((7, 7), dtype=int)],
    )

    # First route is 0 -> 3 -> 4 -> 0 -> 0 -> 5 -> 2 -> 6 -> 0.
    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(Node(loc=3))
    route1.append(Node(loc=4))
    route1.add_trip()
    route1.append(Node(loc=5))
    route1.append(Node(loc=2))
    route1.append(Node(loc=6))
    route1.update()

    # Second route is 0 -> 1 -> 0.
    route2 = Route(data, idx=1, vehicle_type=1)
    route2.append(Node(loc=1))
    route2.update()

    assert_(route1.has_excess_load())
    assert_equal(route1.excess_load(), [15])
    assert_(not route2.has_excess_load())
    assert_equal(route2.excess_load(), [0])

    op = SwapTails(data)
    cost_eval = CostEvaluator(1, 0, 0)

    # The first route becomes: 0 -> 3 -> 1 -> 0. The second route becomes:
    # 0 -> 4 -> 0 -> 0 -> 5 -> 2 -> 6 -> 0.
    # The first trip of the first route has an excess load of 5, where the
    # second trip has no excess load. Thus delta cost would have been -10, but
    # the second route can only have one trip so this move evaluates to 0.
    assert_equal(op.evaluate(route1[1], route2[0], cost_eval), 0)


def test_move_multiple_trips_with_different_depots():
    """
    Tests that SwapTails correctly evaluates moves concerning multiple trips
    where the routes have different depots.
    """
    data = ProblemData(
        clients=[
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
            Client(x=0, y=0, delivery=[5]),
        ],
        depots=[Depot(x=0, y=0), Depot(x=1, y=1)],
        vehicle_types=[
            VehicleType(capacity=[5], max_trips=2, start_depot=0, end_depot=0),
            VehicleType(
                capacity=[10], max_trips=2, start_depot=1, end_depot=1
            ),
        ],
        distance_matrices=[np.zeros((8, 8), dtype=int)],
        duration_matrices=[np.zeros((8, 8), dtype=int)],
    )

    # First route is 0 -> 4 -> 5 -> 0 -> 0 -> 6 -> 7 -> 0.
    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(Node(loc=4))
    route1.append(Node(loc=5))
    route1.add_trip()
    route1.append(Node(loc=6))
    route1.append(Node(loc=7))
    route1.update()

    # Second route is 1 -> 2 -> 1 -> 1 -> 3 -> 1.
    route2 = Route(data, idx=1, vehicle_type=1)
    route2.append(Node(loc=2))
    route2.add_trip()
    route2.append(Node(loc=3))
    route2.update()

    assert_(route1.has_excess_load())
    assert_equal(route1.excess_load(), [10])  # Both trips have 5 excess load
    assert_(not route2.has_excess_load())
    assert_equal(route2.excess_load(), [0])

    op = SwapTails(data)
    cost_eval = CostEvaluator(1, 0, 0)

    # It is not allowed to swap the tails of routes with different depots when
    # at least one of the tails consists of multiple trips.
    assert_equal(op.evaluate(route1[1], route2[0], cost_eval), 0)

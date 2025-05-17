import pickle
from copy import copy, deepcopy

import numpy as np
import pytest
from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

from pyvrp import (
    Client,
    ClientGroup,
    Depot,
    ProblemData,
    RandomNumberGenerator,
    Route,
    Solution,
    Trip,
    VehicleType,
)
from tests.helpers import read


@pytest.mark.parametrize(
    "routes",
    [
        [[3, 4], [1, 2], []],
        [[3, 4], [], [1, 2]],
    ],
)
def test_route_constructor_raises_for_empty_routes(ok_small, routes):
    """
    Tests that constructing a ``Solution`` with empty routes fails.
    """
    with assert_raises(RuntimeError):
        Solution(ok_small, routes)


def test_route_constructor_with_different_vehicle_types(ok_small):
    """
    Tests that Solution's route constructor respects vehicle types.
    """
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(capacity=[10]),
            VehicleType(2, capacity=[20]),
        ]
    )

    sol = Solution(data, [Route(data, [3, 4], 0), Route(data, [1, 2], 1)])

    # We expect Solution to return routes with the correct vehicle types.
    routes = sol.routes()
    assert_equal(len(routes), 2)

    assert_equal(routes[0].visits(), [3, 4])
    assert_equal(routes[0].vehicle_type(), 0)
    assert_equal(routes[0], Route(data, [3, 4], 0))

    assert_equal(routes[1].visits(), [1, 2])
    assert_equal(routes[1].vehicle_type(), 1)
    assert_equal(routes[1], Route(data, [1, 2], 1))


def test_random_constructor_cycles_over_routes(ok_small):
    """
    Tests that a randomly constructed solution fills all available vehicles
    in turn.
    """
    # The OkSmall instance has four clients and three vehicles. Since 1 client
    # per vehicle would not work (insufficient vehicles), each route is given
    # two clients (and the last route should be empty).
    rng = RandomNumberGenerator(seed=42)

    sol = Solution.make_random(ok_small, rng)
    routes = sol.routes()

    assert_equal(sol.num_routes(), 2)
    assert_equal(len(routes), 2)

    for idx, size in enumerate([2, 2]):
        assert_equal(len(routes[idx]), size)


@pytest.mark.parametrize("num_vehicles", (4, 5, 1_000))
def test_random_constructor_uses_all_routes(ok_small, num_vehicles):
    """
    Tests that the randomly constructed solution has exactly as many routes as
    the number of clients when there are sufficient vehicles available.
    """
    data = ok_small.replace(
        vehicle_types=[VehicleType(num_vehicles, capacity=[10])]
    )
    assert_equal(data.num_clients, 4)

    rng = RandomNumberGenerator(seed=42)
    sol = Solution.make_random(data, rng)
    routes = sol.routes()

    for route in routes:
        assert_equal(len(route), 1)

    assert_equal(sol.num_routes(), data.num_clients)
    assert_equal(len(routes), data.num_clients)


def test_random_constructor_uses_all_vehicle_types(ok_small):
    """
    Tests that vehicles of each available vehicle type are used when there are
    more vehicles than clients.
    """
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(ok_small.num_clients, capacity=[10]),
            VehicleType(ok_small.num_clients, capacity=[10]),
        ]
    )

    rng = RandomNumberGenerator(seed=42)
    sol = Solution.make_random(data, rng)

    used_vehicle_types = {route.vehicle_type() for route in sol.routes()}
    assert_equal(used_vehicle_types, {0, 1})


def test_random_constructor_randomly_selects_optional_clients(
    ok_small_prizes,
):
    """
    Tests that the randomly constructed solution selects optional clients
    uniformly at random.
    """
    rng = RandomNumberGenerator(seed=42)

    sols = [Solution.make_random(ok_small_prizes, rng) for _ in range(100)]
    avg_num_clients = np.mean([sol.num_clients() for sol in sols])

    # There are 4 clients, three of which are optional, so the average number
    # of clients should be close to 2.5.
    assert_allclose(avg_num_clients, 2.5, atol=0.1)


def test_route_constructor_raises_too_many_vehicles(ok_small):
    """
    Tests that constructing a solution with more routes than available in the
    data instance raises.
    """
    assert_equal(ok_small.num_vehicles, 3)

    # Only two routes should not raise.
    sol = Solution(ok_small, [[1, 2], [4, 3]])
    assert_equal(len(sol.routes()), 2)

    # Three routes should not raise.
    Solution(ok_small, [[1, 2], [4], [3]])

    # More than three routes should raise, since we only have three vehicles.
    with assert_raises(RuntimeError):
        Solution(ok_small, [[1], [2], [3], [4]])

    # Now test the case with multiple vehicle types.
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(2, capacity=[10]),
            VehicleType(capacity=[20]),
        ]
    )

    # Only two routes (of type 0) should not raise.
    sol = Solution(data, [[1, 2], [4, 3]])
    assert_equal(len(sol.routes()), 2)

    # One route of both vehicle types should not raise.
    sol = Solution(data, [Route(data, [1, 2], 0), Route(data, [4, 3], 1)])
    assert_equal(len(sol.routes()), 2)

    # Two routes of type 1 and one of type 2 should not raise as we have those.
    sol = Solution(
        data,
        [Route(data, [1], 0), Route(data, [2], 0), Route(data, [4, 3], 1)],
    )
    assert_equal(len(sol.routes()), 3)

    # Two routes of vehicle type 1 should raise since we have only one.
    with assert_raises(RuntimeError):
        sol = Solution(data, [Route(data, [1, 2], 1), Route(data, [4, 3], 1)])

    # Three routes should raise since they are considered to be type 0.
    with assert_raises(RuntimeError):
        Solution(data, [[1, 2], [4], [3]])


def test_route_constructor_raises_for_multiple_visits(ok_small):
    """
    Tests that visiting the same client more than once raises.
    """
    with assert_raises(RuntimeError):
        Solution(ok_small, [[1, 2], [1, 3, 4]])  # client 1 is visited twice

    with assert_raises(RuntimeError):
        Solution(
            ok_small, [[1, 2], [1, 3, 4], [1]]
        )  # client 1 is visited thrice


def test_route_constructor_allows_incomplete_solutions(ok_small_prizes):
    """
    Tests that not visiting a client at all is allowed, but turns the solution
    infeasible (unless the client is not required). Allowing this is useful
    for both prize collecting, and in LNS settings where an incomplete solution
    is subsequently repaired.
    """
    # Client 1 is required but not visited.
    sol = Solution(ok_small_prizes, [[2], [3, 4]])
    assert_(not sol.is_complete())
    assert_(not sol.is_feasible())

    # All required clients are visited, but the solution is not feasible.
    sol = Solution(ok_small_prizes, [[1], [2, 3, 4]])
    assert_(not sol.is_feasible())
    assert_(sol.is_complete())

    # All required clients are visited and the solution is feasible.
    sol = Solution(ok_small_prizes, [[1]])
    assert_(sol.is_feasible())
    assert_(sol.is_complete())


def test_neighbours(ok_small):
    """
    Tests that the neighbour structure of (pred, succ) pairs for each client in
    the solution works correctly.
    """
    assert_equal(ok_small.num_clients, 4)

    sol = Solution(ok_small, [[3], [1, 2]])  # client 4 is not in the solution
    assert_(not sol.is_complete())

    neighbours = sol.neighbours()
    expected = [
        None,  # 0: is depot
        (0, 2),  # 1: between depot (0) and 2
        (1, 0),  # 2: between 1 and depot (0)
        (0, 0),  # 3: between depot (0) and depot (0)
        None,  # 4: unassigned
    ]

    for loc in range(ok_small.num_locations):
        assert_equal(neighbours[loc], expected[loc])


def test_neighbours_includes_reload_depots(ok_small_multiple_trips):
    """
    Tests that the neighbours calculation accounts for reload depots.
    """
    trip1 = Trip(ok_small_multiple_trips, [1, 2], 0)
    trip2 = Trip(ok_small_multiple_trips, [3, 4], 0)
    route = Route(ok_small_multiple_trips, [trip1, trip2], 0)

    sol = Solution(ok_small_multiple_trips, [route])
    assert_equal(sol.num_routes(), 1)
    assert_equal(sol.num_trips(), 2)

    neighbours = sol.neighbours()
    assert_(neighbours[0] is None)  # is depot
    assert_equal(neighbours[1], (0, 2))  # (start, 2)
    assert_equal(neighbours[2], (1, 0))  # (1, reload)
    assert_equal(neighbours[3], (0, 4))  # (reload, 4)
    assert_equal(neighbours[4], (3, 0))  # (3, end)


def test_neighbours_multi_depot(ok_small):
    """
    Tests that the neighbour structure of (pred, succ) pairs for each client in
    the solution works correctly when there are multiple depots.
    """
    # Make a two-depot instance by changing the first client in ok_small into
    # a depot, and adding a vehicle type that operates out of that depot.
    locations = ok_small.depots() + ok_small.clients()
    locations[1] = Depot(locations[1].x, locations[1].y)

    data = ok_small.replace(
        depots=locations[:2],
        clients=locations[2:],
        vehicle_types=[
            VehicleType(capacity=[10], start_depot=0, end_depot=0),
            VehicleType(capacity=[10], start_depot=1, end_depot=1),
        ],
    )

    sol = Solution(data, [Route(data, [4], 0), Route(data, [2, 3], 1)])
    assert_(sol.is_complete())

    neighbours = sol.neighbours()
    expected = [
        None,  # 0: is depot
        None,  # 1: is depot
        (1, 3),  # 2: between depot (1) and 3
        (2, 1),  # 3: between 2 and depot (1)
        (0, 0),  # 4: between depot (0) and depot (0)
    ]

    for loc in range(ok_small.num_locations):
        assert_equal(neighbours[loc], expected[loc])


def test_feasibility(ok_small):
    """
    Tests that solutions are infeasible when they have load or time window
    violations.
    """
    # This solution is infeasible due to both load and time window violations.
    sol = Solution(ok_small, [[1, 2, 3, 4]])
    assert_(not sol.is_feasible())

    # First route has total load 18, but vehicle capacity is only 10.
    assert_(sol.has_excess_load())

    # Client 4 has TW [8400, 15300], but client 2 cannot be visited before
    # 15600, so there must be time warp on the single-route solution.
    assert_(sol.has_time_warp())

    # Let's try another solution that's actually feasible.
    sol = Solution(ok_small, [[1, 2], [3], [4]])
    assert_(sol.is_feasible())
    assert_(not sol.has_excess_load())
    assert_(not sol.has_time_warp())


def test_feasibility_release_times():
    """
    Tests solutions can be infeasible due to release time violations, which
    adds time warp.
    """
    data = read("data/OkSmallReleaseTimes.txt")

    # Client 1 is released at 20'000, but client 2 time window ends at 19'500,
    # so this solution must be infeasible due to time-warping. We arrive at
    # client 1 at time 20'000 + 1'544 = 21'544 before the TW closes (22'500).
    # We arrive at client 2 at 21'544 + 360 + 1'992 = 23'896, so we incur a
    # time warp of 23'896 - 19'500 = 4'396.
    sol = Solution(data, [[1, 2], [3], [4]])
    assert_(not sol.is_feasible())
    assert_equal(sol.time_warp(), 4396)

    # Visiting clients 2 and 3 together is feasible: both clients are released
    # at time 5'000. We arrive at client 2 at 5'000 + 1'944 and wait till the
    # TW opens (12'000). We arrive at client 3 at 12'000 + 360 + 621 = 12'981,
    # which is before the TW closes (15'300).
    sol = Solution(data, [[1], [2, 3], [4]])
    assert_(sol.is_feasible())


def test_feasibility_max_duration(ok_small):
    """
    Tests that the maximum duration constraint can affect the feasibility of
    particular solutions.
    """
    # First check that these two routes are feasible when there is no maximum
    # duration constraint.
    sol = Solution(ok_small, [[1, 2], [3, 4]])
    assert_(sol.is_feasible())

    # Modify the data to impose a maximum route duration constraint of 3'000,
    # and check that the previously feasible solution is now not feasible.
    vehicle_type = VehicleType(4, capacity=[10], max_duration=3_000)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    sol = Solution(data, [[1, 2], [3, 4]])
    routes = sol.routes()

    # First route has duration 6'221, and the second route duration 5'004.
    # Since the maximum duration is 3'000, these routes incur time warp of
    # 3'221 + 2'004 = 5'225, and the solution is thus no longer feasible.
    assert_equal(routes[0].duration(), 6_221)
    assert_equal(routes[1].duration(), 5_004)
    assert_equal(sol.time_warp(), 5_225)

    assert_(not routes[0].is_feasible())
    assert_(not routes[1].is_feasible())
    assert_(not sol.is_feasible())


def test_feasibility_max_distance(ok_small):
    """
    Tests that the maximum distance constraint affects solution and route
    feasibility when it is violated.
    """
    sol = Solution(ok_small, [[1, 2], [3, 4]])
    assert_(sol.is_feasible())

    vehicle_type = VehicleType(4, capacity=[10], max_distance=5_000)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    sol = Solution(data, [[1, 2], [3, 4]])
    routes = sol.routes()

    assert_equal(routes[0].distance(), 5501)
    assert_equal(routes[0].excess_distance(), 501)
    assert_(not routes[0].has_time_warp())
    assert_(not routes[0].is_feasible())

    assert_equal(routes[1].distance(), 4224)
    assert_equal(routes[1].excess_distance(), 0)
    assert_(routes[1].is_feasible())

    assert_equal(sol.excess_distance(), 501)
    assert_(sol.has_excess_distance())
    assert_(not sol.is_feasible())


def test_distance_calculation(ok_small):
    """
    Tests that route distance calculations are correct, and that the overall
    Solution's distance is the sum of the route distances.
    """
    sol = Solution(ok_small, [[1, 2], [3], [4]])
    routes = sol.routes()

    # Solution is feasible, so all its routes should also be feasible.
    assert_(sol.is_feasible())
    assert_(all(route.is_feasible() for route in routes))

    # Solution distance should be equal to all routes' distances. These we
    # check separately.
    assert_equal(sol.distance(), sum(route.distance() for route in routes))

    distances = ok_small.distance_matrix(profile=0)
    expected = distances[0, 1] + distances[1, 2] + distances[2, 0]
    assert_equal(routes[0].distance(), expected)

    expected = distances[0, 3] + distances[3, 0]
    assert_equal(routes[1].distance(), expected)

    expected = distances[0, 4] + distances[4, 0]
    assert_equal(routes[2].distance(), expected)


def test_excess_load_calculation(ok_small):
    """
    Tests the Solution's excess load calculation on a single-route case.
    """
    sol = Solution(ok_small, [[4, 3, 1, 2]])
    assert_(sol.has_excess_load())
    assert_(not sol.has_time_warp())

    # All clients are visited on the same route/by the same vehicle. The total
    # delivery demand is 18, but the vehicle capacity is only 10.
    assert_equal(ok_small.num_load_dimensions, 1)

    needed = sum(client.delivery[0] for client in ok_small.clients())
    assert_equal(needed, 18)

    available = ok_small.vehicle_type(0).capacity[0]
    assert_equal(available, 10)

    assert_equal(sol.excess_load(), [needed - available])


@pytest.mark.parametrize(
    ("delivery1", "delivery2", "capacity", "expected_excess_load"),
    [
        ([10, 1], [1, 10], [5, 5], [6, 6]),
        ([1, 10], [10, 1], [5, 5], [6, 6]),
        ([10, 10], [1, 1], [5, 5], [6, 6]),
        ([10, 1], [1, 10], [11, 5], [0, 6]),
        ([10, 1], [1, 10], [5, 11], [6, 0]),
    ],
)
def test_excess_load_calculation_with_multiple_load_dimensions(
    delivery1,
    delivery2,
    capacity,
    expected_excess_load,
):
    """
    Tests several cases with multiple load dimensions to ensure excess load is
    calculated correctly.
    """
    data = ProblemData(
        clients=[
            Client(1, 0, delivery1),
            Client(2, 0, delivery2),
        ],
        depots=[Depot(0, 0)],
        vehicle_types=[VehicleType(1, capacity=capacity)],
        distance_matrices=[[[0, 1, 2], [1, 0, 1], [2, 1, 0]]],
        duration_matrices=[[[0, 1, 2], [1, 0, 1], [2, 1, 0]]],
    )
    solution = Solution(data, [[1, 2]])

    assert_(solution.has_excess_load())
    assert_equal(solution.excess_load(), expected_excess_load)


@pytest.mark.parametrize(
    "dist_mat",
    [
        np.where(np.eye(3), 0, 100),
        np.where(np.eye(3), 0, 1),
        np.where(np.eye(3), 0, 1000),
    ],
)
def test_time_warp_for_a_very_constrained_problem(dist_mat):
    """
    This tests an artificial instance where the second client cannot be reached
    directly from the depot in a feasible solution, but only after the first
    client.
    """
    dur_mat = [
        [0, 1, 10],  # cannot get to 2 from depot within 2's time window
        [1, 0, 1],
        [1, 1, 0],
    ]

    data = ProblemData(
        clients=[
            Client(x=1, y=0, tw_late=5),
            Client(x=2, y=0, tw_late=5),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(2, tw_late=10)],
        distance_matrices=[dist_mat],
        duration_matrices=[dur_mat],
    )

    # This solution directly visits the second client from the depot, which is
    # not time window feasible.
    infeasible = Solution(data, [[1], [2]])
    assert_(infeasible.has_time_warp())
    assert_(not infeasible.has_excess_load())
    assert_(not infeasible.is_feasible())

    # But visiting the second client after the first is feasible.
    feasible = Solution(data, [[1, 2]])
    assert_(not feasible.has_time_warp())
    assert_(not feasible.has_excess_load())
    assert_(feasible.is_feasible())

    assert_equal(
        feasible.distance(),
        dist_mat[0, 1] + dist_mat[1, 2] + dist_mat[2, 0],
    )


def test_time_warp_return_to_depot():
    """
    This tests whether the calculated total duration and time warp includes the
    travel back to the depot.
    """
    data = ProblemData(
        clients=[Client(x=1, y=0)],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(tw_late=1)],
        distance_matrices=[np.asarray([[0, 0], [0, 0]])],
        duration_matrices=[np.asarray([[0, 1], [1, 0]])],
    )

    sol = Solution(data, [[1]])
    route, *_ = sol.routes()

    # Travel from depot to client and back gives duration 1 + 1 = 2. This is 1
    # more than the vehicle time window 1, giving a time warp of 1.
    assert_equal(route.duration(), 2)
    assert_equal(data.vehicle_type(0).tw_late, 1)
    assert_equal(sol.time_warp(), 1)


def tests_that_not_specifying_the_vehicle_type_assumes_a_default(ok_small):
    """
    Not specifying the vehicle type when providing a list of visits uses the
    first vehicle type to complete the routes. That could result in a solution
    using too many vehicles of the first type.
    """
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(2, capacity=[10]),
            VehicleType(capacity=[20]),
        ]
    )

    sol = Solution(data, [[1, 2, 3, 4]])
    assert_equal(sol.num_routes(), 1)

    sol = Solution(data, [Route(data, [1, 2, 3, 4], 0)])
    assert_equal(sol.num_routes(), 1)

    sol = Solution(data, [[1, 2], [3, 4]])
    assert_equal(sol.num_routes(), 2)

    with assert_raises(RuntimeError):
        # This raises since we don't specify route types, which means we create
        # 3 routes of type 0 whereas we only have 2 available.
        sol = Solution(data, [[1], [2], [3, 4]])

    # It works if we specify the correct vehicle types.
    sol = Solution(
        data,
        [Route(data, [1], 0), Route(data, [2], 0), Route(data, [3, 4], 1)],
    )
    assert_equal(sol.num_routes(), 3)

    # But not if we violate the number of available vehicles of that type.
    with assert_raises(RuntimeError):
        sol = Solution(
            data,
            [Route(data, [1], 0), Route(data, [2], 1), Route(data, [3, 4], 1)],
        )


def test_copy(ok_small):
    """
    Tests that copied solutions are equal to the original solution, but not
    the exact same object.
    """
    sol = Solution(ok_small, [[1, 2, 3, 4]])
    copy_sol = copy(sol)
    deepcopy_sol = deepcopy(sol)

    # Copied solutions are equal to the original solution.
    assert_(sol == copy_sol)
    assert_(sol == deepcopy_sol)

    # But they are not the same object.
    assert_(sol is not copy_sol)
    assert_(sol is not deepcopy_sol)


def test_eq(ok_small):
    """
    Tests the solution's equality operator.
    """
    sol1 = Solution(ok_small, [[1, 2, 3, 4]])
    sol2 = Solution(ok_small, [[1, 2], [3], [4]])
    sol3 = Solution(ok_small, [[1, 2, 3, 4]])

    assert_(sol1 == sol1)  # Solutions should be equal to themselves
    assert_(sol2 == sol2)
    assert_(sol1 != sol2)  # different routes, so should not be equal
    assert_(sol1 == sol3)  # same routes, different solution

    sol4 = Solution(ok_small, [[1, 2, 3], [4]])
    sol5 = Solution(ok_small, [[4], [1, 2, 3]])

    assert_(sol4 == sol5)  # routes are the same, but in different order

    # And a few tests against things that are not solutions, just to be sure
    # there's also a type check in there somewhere.
    assert_(sol4 != 1)
    assert_(sol4 != "abc")
    assert_(sol5 != 5)
    assert_(sol5 != "cd")


def test_eq_with_multiple_vehicle_types(ok_small):
    """
    Tests that two solutions are not considered equal if they have the same
    routes (orders of clients) but served by different vehicle types.
    """
    # Make sure capacities are different but large enough (>18) to have no
    # violations so have the same attributes, such that we actually test if the
    # assignments are used for the equality comparison.
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(2, capacity=[20]),
            VehicleType(capacity=[30]),
        ]
    )

    # These two should be the same
    sol1 = Solution(data, [[1, 2, 3, 4]])
    sol2 = Solution(data, [Route(data, [1, 2, 3, 4], 0)])
    # Create solution with different vehicle type
    sol3 = Solution(data, [Route(data, [1, 2, 3, 4], 1)])

    # First two solution have one route with the same vehicle type
    assert_(sol1 == sol2)
    # Solution 3 is different since the route has a different vehicle type
    assert_(sol1 != sol3)

    # Order should not matter so these should be the same
    sol1 = Solution(data, [Route(data, [1, 2], 0), Route(data, [3, 4], 1)])
    sol2 = Solution(data, [Route(data, [3, 4], 1), Route(data, [1, 2], 0)])
    assert_(sol1 == sol2)

    # But changing the vehicle types should be different
    sol3 = Solution(data, [Route(data, [1, 2], 1), Route(data, [3, 4], 0)])
    assert_(sol1 != sol3)


def test_eq_unassigned():
    """
    Tests the equality operator for solutions with unassigned clients.
    """
    dist = [[0, 1, 1], [1, 0, 1], [1, 1, 0]]
    data = ProblemData(
        clients=[
            Client(x=0, y=1, required=False),
            Client(x=1, y=0, required=False),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(2)],
        distance_matrices=[dist],
        duration_matrices=[dist],
    )

    sol1 = Solution(data, [[1]])
    sol2 = Solution(data, [[1]])
    sol3 = Solution(data, [[2]])

    assert_(sol1 == sol2)
    assert_(sol1 != sol3)


def test_duplicate_vehicle_types(ok_small):
    """
    Tests that it is allowed to have duplicate vehicle types. These will be
    considered completely different during optimisation.
    """
    data = ok_small.replace(
        vehicle_types=[VehicleType(capacity=[10]), VehicleType(capacity=[10])]
    )

    sol1 = Solution(data, [Route(data, [1, 2, 3, 4], 0)])
    sol2 = Solution(data, [Route(data, [1, 2, 3, 4], 1)])

    assert_(sol1 != sol2)


@pytest.mark.parametrize(
    "vehicle_types",
    [
        [VehicleType(3, capacity=[10])],
        [VehicleType(2, capacity=[10]), VehicleType(capacity=[20])],
    ],
)
def test_str_contains_routes(ok_small, vehicle_types):
    """
    Tests that the Solution's string representation contains each route.
    """
    data = ok_small.replace(vehicle_types=vehicle_types)
    rng = RandomNumberGenerator(seed=2)

    for _ in range(5):  # let's do this a few times to really make sure
        sol = Solution.make_random(data, rng)
        str_representation = str(sol).splitlines()
        routes = sol.routes()

        # There should be no more than len(routes) lines (each detailing a
        # single route).
        assert_equal(len(str_representation), len(routes))

        # Each line should contain a route, where each route should contain
        # every client that is in the route as returned by routes().
        for route, str_route in zip(routes, str_representation):
            for client in route:
                assert_(str(client) in str_route)


def test_hash(ok_small):
    """
    Tests that solutions that compare the same have the same hash.
    """
    rng = RandomNumberGenerator(seed=2)

    sol1 = Solution.make_random(ok_small, rng)
    sol2 = Solution.make_random(ok_small, rng)

    # Two random solutions. They're not the same, so the hashes should not be
    # the same either (unless there's a collision, which is not the case here).
    assert_(sol1 != sol2)
    assert_(hash(sol1) != hash(sol2))

    sol3 = deepcopy(sol2)  # is a direct copy

    # These two are the same solution, so their hashes should be the same too.
    assert_equal(sol2, sol3)
    assert_equal(hash(sol2), hash(sol3))


def test_solution_can_be_pickled(ok_small):
    """
    Tests that a solution can be serialised and unserialised.
    """
    rng = RandomNumberGenerator(seed=2)

    before_pickle = Solution.make_random(ok_small, rng)
    bytes = pickle.dumps(before_pickle)
    after_pickle = pickle.loads(bytes)

    assert_equal(after_pickle, before_pickle)


@pytest.mark.parametrize(
    ("assignment", "expected"), [((0, 0), 0), ((0, 1), 10), ((1, 1), 20)]
)
def test_fixed_vehicle_cost(
    ok_small, assignment: tuple[int, int], expected: int
):
    """
    Tests that the solution tracks the total fixed vehicle costs of the
    vehicles used for its routes.
    """
    # First vehicle type is free, second costs 10 per vehicle. The solution
    # should be able to track this.
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(2, capacity=[10], fixed_cost=0),
            VehicleType(2, capacity=[10], fixed_cost=10),
        ]
    )

    routes = [
        Route(data, [1, 2], assignment[0]),
        Route(data, [3, 4], assignment[1]),
    ]

    sol = Solution(data, routes)
    assert_equal(sol.fixed_vehicle_cost(), expected)


@pytest.mark.parametrize(
    ("routes", "feasible"),
    [
        ([[1], [3, 4]], True),  # only one - OK
        ([[2], [3, 4]], True),  # only one - OK
        ([[1, 2], [3, 4]], False),  # both are in the solution - not OK
        ([[3, 4]], False),  # none - not OK
    ],
)
def test_solution_feasibility_with_mutually_exclusive_groups(
    ok_small, routes: list[list[int]], feasible: bool
):
    """
    Tests that the Solution class correctly accounts for feasibility regarding
    any mutually exclusive groups in the data.
    """
    # Clients 1 and 2 are part of a mutually exclusive group. Of these clients,
    # exactly one must be part of a feasible solution.
    clients = ok_small.clients()
    clients[0] = Client(1, 1, delivery=[0], required=False, group=0)
    clients[1] = Client(2, 2, delivery=[0], required=False, group=0)

    group = ClientGroup([1, 2], required=True)
    assert_(group.required)
    assert_(group.mutually_exclusive)

    data = ok_small.replace(clients=clients, groups=[group])
    sol = Solution(data, routes)
    assert_equal(sol.is_feasible(), feasible)
    assert_equal(sol.is_group_feasible(), feasible)


def test_optional_mutually_exclusive_group(ok_small):
    """
    Tests that mutually exclusive client groups can be skipped if they are
    not required. In that case at most one client from the group needs to be
    in the solution, but zero is also OK.
    """
    # Clients 1 and 2 are part of a mutually exclusive group. Of these clients,
    # at most one must be part of a feasible solution.
    clients = ok_small.clients()
    clients[0] = Client(1, 1, delivery=[0], required=False, group=0)
    clients[1] = Client(2, 2, delivery=[0], required=False, group=0)

    group = ClientGroup([1, 2], required=False)
    assert_(not group.required)
    assert_(group.mutually_exclusive)

    data = ok_small.replace(clients=clients, groups=[group])
    sol = Solution(data, [[3, 4]])
    assert_(sol.is_feasible())
    assert_(sol.is_group_feasible())


def test_distance_duration_cost_calculations(ok_small):
    """
    Tests solution-level distance and duration cost calculations.
    """
    vehicle_types = [
        VehicleType(capacity=[10], unit_distance_cost=5, unit_duration_cost=1),
        VehicleType(capacity=[10], unit_distance_cost=1, unit_duration_cost=5),
    ]
    data = ok_small.replace(vehicle_types=vehicle_types)
    routes = [Route(data, [1, 2], 0), Route(data, [3, 4], 1)]

    sol = Solution(data, routes)
    assert_equal(sol.distance(), sum(r.distance() for r in routes))
    assert_equal(sol.distance_cost(), sum(r.distance_cost() for r in routes))
    assert_equal(sol.duration(), sum(r.duration() for r in routes))
    assert_equal(sol.duration_cost(), sum(r.duration_cost() for r in routes))

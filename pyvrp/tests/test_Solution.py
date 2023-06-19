from copy import copy, deepcopy

import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises
from pytest import mark

from pyvrp import Client, ProblemData, Route, Solution, XorShift128
from pyvrp.tests.helpers import read


def test_route_constructor_filters_empty():
    data = read("data/OkSmall.txt")

    sol = Solution(data, [[3, 4], [], [1, 2]])
    routes = sol.get_routes()

    # num_routes() and len(routes) should show two non-empty routes.
    assert_equal(sol.num_routes(), 2)
    assert_equal(len(routes), 2)

    # The only two non-empty routes should now each have two clients.
    assert_equal(len(routes[0]), 2)
    assert_equal(len(routes[1]), 2)


def test_random_constructor_cycles_over_routes():
    # This instance has four clients and three vehicles. Since 1 client per
    # vehicle would not work (insufficient vehicles), each route is given two
    # clients (and the last route should be empty).
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=42)

    sol = Solution.make_random(data, rng)
    routes = sol.get_routes()

    assert_equal(sol.num_routes(), 2)
    assert_equal(len(routes), 2)

    for idx, size in enumerate([2, 2]):
        assert_equal(len(routes[idx]), size)


def test_route_constructor_raises_too_many_vehicles():
    data = read("data/OkSmall.txt")

    assert_equal(data.num_vehicles, 3)

    # Only two routes should not raise.
    sol = Solution(data, [[1, 2], [4, 3]])
    assert_equal(len(sol.get_routes()), 2)

    # Empty third route should not raise.
    Solution(data, [[1, 2], [4, 3], []])

    # More than three routes should raise, since we only have three vehicles.
    with assert_raises(RuntimeError):
        Solution(data, [[1], [2], [3], [4]])


def test_route_constructor_raises_for_invalid_routes():
    data = read("data/OkSmall.txt")
    with assert_raises(RuntimeError):
        Solution(data, [[1, 2], [1, 3, 4]])  # client 1 is visited twice

    data = read("data/OkSmallPrizes.txt")
    with assert_raises(RuntimeError):
        Solution(data, [[2], [3, 4]])  # 1 is required but not visited


def test_get_neighbours():
    data = read("data/OkSmall.txt")

    sol = Solution(data, [[3, 4], [], [1, 2]])
    neighbours = sol.get_neighbours()

    expected = [
        (0, 0),  # 0: is depot
        (0, 2),  # 1: between depot (0) to 2
        (1, 0),  # 2: between 1 and depot (0)
        (0, 4),  # 3: between depot (0) and 4
        (3, 0),  # 4: between 3 and depot (0)
    ]

    assert_equal(data.num_clients, 4)

    for client in range(data.num_clients + 1):  # incl. depot
        assert_equal(neighbours[client], expected[client])


def test_feasibility():
    data = read("data/OkSmall.txt")

    # This solution is infeasible due to both load and time window violations.
    sol = Solution(data, [[1, 2, 3, 4]])
    assert_(not sol.is_feasible())

    # First route has total load 18, but vehicle capacity is only 10.
    assert_(sol.has_excess_load())

    # Client 4 has TW [8400, 15300], but client 2 cannot be visited before
    # 15600, so there must be time warp on the single-route solution.
    assert_(sol.has_time_warp())

    # Let's try another solution that's actually feasible.
    sol = Solution(data, [[1, 2], [3], [4]])
    assert_(sol.is_feasible())
    assert_(not sol.has_excess_load())
    assert_(not sol.has_time_warp())


def test_distance_calculation():
    data = read("data/OkSmall.txt")

    sol = Solution(data, [[1, 2], [3], [4]])
    routes = sol.get_routes()

    # Solution is feasible, so all its routes should also be feasible.
    assert_(sol.is_feasible())
    assert_(all(route.is_feasible() for route in routes))

    # Solution distance should be equal to all routes' distances. These we
    # check separately.
    assert_allclose(sol.distance(), sum(route.distance() for route in routes))

    expected = data.dist(0, 1) + data.dist(1, 2) + data.dist(2, 0)
    assert_allclose(routes[0].distance(), expected)

    expected = data.dist(0, 3) + data.dist(3, 0)
    assert_allclose(routes[1].distance(), expected)

    expected = data.dist(0, 4) + data.dist(4, 0)
    assert_allclose(routes[2].distance(), expected)


def test_excess_load_calculation():
    data = read("data/OkSmall.txt")

    sol = Solution(data, [[4, 3, 1, 2]])
    assert_(sol.has_excess_load())
    assert_(not sol.has_time_warp())

    # All clients are visited on the same route/by the same vehicle. The total
    # demand is 18, but the vehicle capacity is only 10. This has a non-zero
    # load penalty
    assert_equal(sol.excess_load(), 18 - data.vehicle_capacity)


def test_route_access_methods():
    data = read("data/OkSmall.txt")
    sol = Solution(data, [[1, 3], [2, 4]])
    routes = sol.get_routes()

    # Test route access: getting the route plan should return a simple list, as
    # given to the solution above.
    assert_equal(routes[0].visits(), [1, 3])
    assert_equal(routes[1].visits(), [2, 4])

    # There's no excess load, so all excess load should be zero.
    assert_(not sol.has_excess_load())
    assert_allclose(routes[0].excess_load(), 0)
    assert_allclose(routes[1].excess_load(), 0)

    # Total route demand.
    demands = [data.client(idx).demand for idx in range(data.num_clients + 1)]
    assert_allclose(routes[0].demand(), demands[1] + demands[3])
    assert_allclose(routes[1].demand(), demands[2] + demands[4])

    # The first route is not feasible due to time warp, but the second one is.
    # See also the tests below.
    assert_(not routes[0].is_feasible())
    assert_(routes[1].is_feasible())

    # Total service duration.
    services = [
        data.client(idx).service_duration
        for idx in range(data.num_clients + 1)
    ]
    assert_allclose(routes[0].service_duration(), services[1] + services[3])
    assert_allclose(routes[1].service_duration(), services[2] + services[4])


def test_route_time_warp_and_start_time_calculations():
    data = read("data/OkSmall.txt")
    sol = Solution(data, [[1, 3], [2, 4]])
    routes = sol.get_routes()

    # There's only time warp on the first route: duration(0, 1) = 1'544, so we
    # arrive at 1 before its opening window of 15'600. Service (360) thus
    # starts at 15'600, and completes at 15'600 + 360. Then we drive for
    # duration(1, 3) = 1'427, where we arrive after 15'300 (its closing time
    # window). This is where we incur time warp: we need to 'warp' to 15'300.
    assert_(sol.has_time_warp())
    assert_(routes[0].has_time_warp())
    assert_(not routes[1].has_time_warp())
    assert_allclose(routes[0].time_warp(), 15_600 + 360 + 1_427 - 15_300)
    assert_allclose(routes[1].time_warp(), 0)
    assert_allclose(sol.time_warp(), routes[0].time_warp())

    # In both routes, there is no waiting time.
    assert_equal(routes[0].wait_duration(), 0)
    assert_equal(routes[1].wait_duration(), 0)

    # Since route 0 has a timewarp, there is no slack and the
    # route should start exactly at 15'600 - 1'544 = 14056
    assert_equal(routes[0].earliest_start(), 14_056)
    assert_equal(routes[0].latest_start(), 14_056)
    assert_equal(routes[0].slack(), 0)

    # Route 1 has some slack, it shouldn't start before 12'000 - 1'944 = 10'056
    # (otherwise we will have waiting time and a longer duration) and not after
    # 19'500 - 1'090 - 360 - 1'944 = 16'106
    assert_equal(routes[1].earliest_start(), 10_056)
    assert_equal(routes[1].latest_start(), 16_106)
    assert_equal(routes[1].slack(), 16_106 - 10_056)


def test_route_wait_time_calculations():
    data = read("data/OkSmallWaitTime.txt")
    sol = Solution(data, [[1, 3], [2, 4]])
    routes = sol.get_routes()

    # In route 1, the latest start of service for client 2 is 15'000, then
    # adding 360 service and 1'090 travel we arrive at client 4 at 16'450 and
    # have to wait 18'000 - 16'450 = 1'550
    assert_equal(routes[1].wait_duration(), 1_550)
    # Since there is waiting time, there is no slack and we should start
    # as late as possible, at 15'000 - 1'944 = 13'056
    assert_equal(routes[1].earliest_start(), 13_056)
    assert_equal(routes[1].latest_start(), 13_056)
    assert_equal(routes[1].slack(), 0)

    # Additionally, we will test that we can have both wait time and time warp
    # in a single route, and it holds that duration = travel + service + wait
    sol = Solution(data, [[1, 3, 2, 4]])
    route = sol.get_routes()[0]

    assert_(route.has_time_warp())
    assert_(route.time_warp() > 0)
    assert_(route.wait_duration() > 0)
    assert_equal(
        route.duration(),
        route.travel_duration()
        + route.service_duration()
        + route.wait_duration(),
    )
    # In this case, there is no slack either
    assert_equal(route.earliest_start(), route.latest_start())
    assert_equal(route.slack(), 0)


@mark.parametrize(
    "dist_mat",
    [
        np.full((3, 3), fill_value=100, dtype=int),
        np.full((3, 3), fill_value=1, dtype=int),
        np.full((3, 3), fill_value=1000, dtype=int),
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
            Client(x=0, y=0, tw_late=10),
            Client(x=1, y=0, tw_late=5),
            Client(x=2, y=0, tw_late=5),
        ],
        num_vehicles=2,
        vehicle_cap=0,
        distance_matrix=dist_mat,
        duration_matrix=dur_mat,
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
    This tests wether the calculated total duration and time warp includes the
    travel back to the depot.
    """
    data = ProblemData(
        clients=[Client(x=0, y=0, tw_late=1), Client(x=1, y=0)],
        num_vehicles=1,
        vehicle_cap=0,
        distance_matrix=[[0, 0], [0, 0]],
        duration_matrix=[[0, 1], [1, 0]],
    )
    # Travel from depot to client and back gives duration 1 + 1 = 2
    # This is 1 more than the depot time window 1, giving a time warp of 1
    sol = Solution(data, [[1]])
    routes = sol.get_routes()
    assert_equal(routes[0].duration(), 2)
    assert_equal(sol.time_warp(), 1)


# TODO test all time warp cases


def test_copy():
    data = read("data/OkSmall.txt")

    sol = Solution(data, [[1, 2, 3, 4]])
    copy_sol = copy(sol)
    deepcopy_sol = deepcopy(sol)

    # Copied solutions are equal to the original solution
    assert_(sol == copy_sol)
    assert_(sol == deepcopy_sol)

    # But they are not the same object
    assert_(sol is not copy_sol)
    assert_(sol is not deepcopy_sol)


def test_eq():
    data = read("data/OkSmall.txt")

    sol1 = Solution(data, [[1, 2, 3, 4]])
    sol2 = Solution(data, [[1, 2], [3], [4]])
    sol3 = Solution(data, [[1, 2, 3, 4]])

    assert_(sol1 == sol1)  # Solutions should be equal to themselves
    assert_(sol2 == sol2)
    assert_(sol1 != sol2)  # different routes, so should not be equal
    assert_(sol1 == sol3)  # same routes, different solution

    sol4 = Solution(data, [[1, 2, 3], [], [4]])
    sol5 = Solution(data, [[4], [1, 2, 3], []])

    assert_(sol4 == sol5)  # routes are the same, but in different order

    # And a few tests against things that are not solutions, just to be sure
    # there's also a type check in there somewhere.
    assert_(sol4 != 1)
    assert_(sol4 != "abc")
    assert_(sol5 != 5)
    assert_(sol5 != "cd")


def test_str_contains_routes():
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=2)

    for _ in range(5):  # let's do this a few times to really make sure
        sol = Solution.make_random(data, rng)
        str_representation = str(sol).splitlines()
        routes = sol.get_routes()

        # There should be no more than len(routes) lines (each detailing a
        # single route).
        assert_equal(len(str_representation), len(routes))

        # Each line should contain a route, where each route should contain
        # every client that is in the route as returned by get_routes().
        for route, str_route in zip(routes, str_representation):
            for client in route:
                assert_(str(client) in str_route)


def test_hash():
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=2)

    sol1 = Solution.make_random(data, rng)
    sol2 = Solution.make_random(data, rng)

    hash1 = hash(sol1)
    hash2 = hash(sol2)

    # Two random solutions. They're not the same, so the hashes should not be
    # the same either.
    assert_(sol1 != sol2)
    assert_(hash1 != hash2)

    sol3 = deepcopy(sol2)  # is a direct copy

    # These two are the same solution, so their hashes should be the same too.
    assert_equal(sol2, sol3)
    assert_equal(hash(sol2), hash(sol3))


def test_route_centroid():
    data = read("data/OkSmall.txt")
    x = np.array([data.client(client).x for client in range(5)])
    y = np.array([data.client(client).y for client in range(5)])

    routes = [Route(data, [1, 2]), Route(data, [3]), Route(data, [4])]

    for route in routes:
        x_center, y_center = route.centroid()
        assert_equal(x_center, x[route].mean())
        assert_equal(y_center, y[route].mean())

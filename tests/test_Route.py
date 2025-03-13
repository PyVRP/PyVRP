import pickle

import numpy as np
import pytest
from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import (
    Client,
    Depot,
    ProblemData,
    RandomNumberGenerator,
    Route,
    Solution,
    VehicleType,
)
from tests.helpers import read


def test_route_depot_accessors(ok_small_multi_depot):
    """
    Tests that Route's start_depot() and end_depot() members return the correct
    depot location indices.
    """
    routes = [
        Route(ok_small_multi_depot, [2], 0),
        Route(ok_small_multi_depot, [3, 4], 1),
    ]

    assert_equal(routes[0].start_depot(), 0)
    assert_equal(routes[0].end_depot(), 0)
    assert_equal(routes[1].start_depot(), 1)
    assert_equal(routes[1].end_depot(), 1)


def test_route_eq(ok_small):
    """
    Tests ``Route``'s equality operator.
    """
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(capacity=[10]),
            VehicleType(2, capacity=[20]),
        ]
    )

    route1 = Route(data, [1, 2], 0)
    assert_(route1 == route1)  # should equal self

    route2 = Route(data, [1, 2], 0)
    assert_equal(route1, route2)  # same route/vehicle type; different object

    route3 = Route(data, [1, 2], 1)
    assert_(route2 != route3)  # different vehicle type

    route4 = Route(data, [2, 1], 0)
    assert_(route2 != route4)  # same vehicle type, different visits

    assert_(route1 != "test")
    assert_(route1 != 0)
    assert_(route1 != -1.0)


def test_route_access_methods(ok_small):
    """
    Tests that accessing route statistics returns the correct numbers.
    """
    routes = [Route(ok_small, [1, 3], 0), Route(ok_small, [2, 4], 0)]

    # Test route access: getting the route plan should return a simple list.
    assert_equal(routes[0].visits(), [1, 3])
    assert_equal(routes[1].visits(), [2, 4])

    # There's no excess load, so all excess load should be zero.
    assert_equal(routes[0].excess_load(), [0])
    assert_equal(routes[1].excess_load(), [0])

    # Total route delivery demand (and pickups, which are all zero for this
    # instance).
    deliveries = [0] + [client.delivery[0] for client in ok_small.clients()]
    assert_equal(routes[0].delivery(), [deliveries[1] + deliveries[3]])
    assert_equal(routes[1].delivery(), [deliveries[2] + deliveries[4]])

    assert_equal(routes[0].pickup(), [0])
    assert_equal(routes[1].pickup(), [0])

    # The first route is not feasible due to time warp, but the second one is.
    # See also the tests below.
    assert_(not routes[0].is_feasible())
    assert_(routes[1].is_feasible())

    # Total service duration.
    services = [0] + [client.service_duration for client in ok_small.clients()]
    assert_equal(routes[0].service_duration(), services[1] + services[3])
    assert_equal(routes[1].service_duration(), services[2] + services[4])


def test_route_time_warp_calculations(ok_small):
    """
    Tests route time warp calculations.
    """
    routes = [Route(ok_small, [1, 3], 0), Route(ok_small, [2, 4], 0)]

    # There is time warp on the first route: duration(0, 1) = 1'544, so we
    # arrive at 1 before its opening window of 15'600. Service (360) thus
    # starts at 15'600, and completes at 15'600 + 360. Then we drive for
    # duration(1, 3) = 1'427, where we arrive after 15'300 (its closing time
    # window). This is where we incur time warp: we need to 'warp' to 15'300.
    assert_(routes[0].has_time_warp())
    assert_equal(routes[0].time_warp(), 15_600 + 360 + 1_427 - 15_300)

    # The second route has no time warp.
    assert_(not routes[1].has_time_warp())
    assert_equal(routes[1].time_warp(), 0)


def test_route_wait_time_calculations():
    """
    Tests route wait time and slack calculations.
    """
    data = read("data/OkSmallWaitTime.txt")
    routes = [Route(data, [1, 3], 0), Route(data, [2, 4], 0)]

    # In the second route, the time window of client 2 closes at 15'000. After
    # service and travel, we then arrive at client 4 before its time window is
    # open, so we have to wait. In particular, we have to wait:
    #   twEarly(4) - duration(2, 4) - serv(2) - twLate(2)
    #     = 18'000 - 1'090 - 360 - 15'000
    #     = 1'550.
    assert_equal(routes[1].wait_duration(), 1_550)

    # Since there is waiting time, there is no slack in the schedule. We should
    # thus start as late as possible, at:
    #   twLate(2) - duration(0, 2)
    #     = 15'000 - 1'944
    #     = 13'056.
    assert_equal(routes[1].slack(), 0)
    assert_equal(routes[1].start_time(), 13_056)

    # So far we have tested a route that had wait duration, but not time warp.
    # The following route has both.
    route = Route(data, [1, 2, 4], 0)

    # This route has the same wait time as explained above. The time warp is
    # incurred earlier in the route, between 1 -> 2:
    #   twEarly(1) + serv(1) + duration(1, 2) - twLate(2)
    #     = 15'600 + 360 + 1'992 - 15'000
    #     = 2'952.
    assert_equal(route.time_warp(), 2_952)
    assert_equal(route.wait_duration(), 1_550)
    assert_equal(route.slack(), 0)

    # Finally, the overall route duration should be equal to the sum of the
    # travel, service, and waiting durations.
    assert_equal(
        route.duration(),
        route.travel_duration()
        + route.service_duration()
        + route.wait_duration(),
    )


def test_route_start_and_end_time_calculations(ok_small):
    """
    Tests route start time, slack, and end time calculations for cases with
    and without time warp or wait duration.
    """
    routes = [Route(ok_small, [1, 3], 0), Route(ok_small, [2, 4], 0)]

    # The first route has timewarp, so there is no slack in the schedule. We
    # should thus depart as soon as possible to arrive at the first client the
    # moment its time window opens.
    durations = ok_small.duration_matrix(profile=0)
    start_time = ok_small.location(1).tw_early - durations[0, 1]
    end_time = start_time + routes[0].duration() - routes[0].time_warp()

    assert_(routes[0].has_time_warp())
    assert_equal(routes[0].slack(), 0)
    assert_equal(routes[0].start_time(), start_time)
    assert_equal(routes[0].end_time(), end_time)

    # The second route has no time warp. The latest it can start is calculated
    # backwards from the closing of client 4's time window:
    #   twLate(4) - duration(2, 4) - serv(2) - duration(0, 2)
    #     = 19'500 - 1'090 - 360 - 1'944
    #     = 16'106.
    #
    # Because client 4 has a large time window, the earliest this route can
    # start is determined completely by client 2: we should not arrive before
    # its time window, because that would incur needless waiting. We should
    # thus not depart before:
    #   twEarly(2) - duration(0, 2)
    #     = 12'000 - 1'944
    #     = 10'056.
    assert_(not routes[1].has_time_warp())
    assert_equal(routes[1].wait_duration(), 0)
    assert_equal(routes[1].start_time(), 10_056)
    assert_equal(routes[1].slack(), 16_106 - 10_056)

    # The overall route duration is given by:
    #   duration(0, 2) + serv(2) + duration(2, 4) + serv(4) + duration(4, 0)
    #     = 1'944 + 360 + 1'090 + 360 + 1'475
    #     = 5'229.
    assert_equal(routes[1].duration(), 1_944 + 360 + 1_090 + 360 + 1_475)
    assert_equal(routes[1].end_time(), 10_056 + 5_229)


def test_route_release_time():
    """
    Tests that routes return the correct release times, and, when feasible,
    start after the release time.
    """
    data = read("data/OkSmallReleaseTimes.txt")
    routes = [Route(data, [1, 3], 0), Route(data, [2, 4], 0)]

    # The client release times are 20'000, 5'000, 5'000 and 1'000. So the first
    # route has a release time of max(20'000, 5'000) = 20'000, and the second
    # has a release time of max(5'000, 1'000) = 5'000.
    assert_equal(routes[0].release_time(), 20_000)
    assert_equal(routes[1].release_time(), 5_000)

    # Second route is feasible, so should have start time not smaller than
    # release time.
    assert_(not routes[1].has_time_warp())
    assert_(routes[1].start_time() > routes[1].release_time())


def test_release_time_and_max_duration():
    """
    Tests the interaction of release times and maximum duration constraints. In
    particular, we verify that the maximum duration applies to the time after
    the vehicle starts their route, and that the release time only shifts
    that starting moment - it does not affect the overall maximum duration.
    """
    ok_small = read("data/OkSmallReleaseTimes.txt")
    vehicle_type = VehicleType(3, [10], max_duration=5_000)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    # This route has a release time of 5000, but should not start until much
    # later because of the time windows. The vehicle's maximum duration is also
    # 5000, but this is violated by 998 units of time. The route is otherwise
    # feasible, so there is exactly 998 time warp.
    route = Route(data, [2, 3, 4], 0)
    assert_equal(route.release_time(), 5_000)
    assert_equal(route.start_time(), 10_056)
    assert_equal(route.end_time(), 15_056)
    assert_equal(route.duration(), 5_998)
    assert_equal(route.service_duration(), 1_140)
    assert_equal(route.time_warp(), 998)


def test_release_time_and_service_duration_duration():
    """
    Tests the interaction between release times and depot service duration, and
    checks that service at the depot happens after the tasks are released. See
    also the ``test_release_time_and_max_duration`` test.
    """
    ok_small = read("data/OkSmallReleaseTimes.txt")
    depot = Depot(x=2334, y=726, service_duration=6_000)
    data = ok_small.replace(depots=[depot])

    # This route has a release time of 5000, but we want to start much later
    # anyway because of the time windows. That's not possible, however, because
    # of the depot service duration of 6000. The overall route duration is 5998
    # plus the 6000, and there is no time warp.
    route = Route(data, [2, 3, 4], 0)
    assert_equal(route.release_time(), 5_000)
    assert_equal(route.start_time(), 5_000)
    assert_equal(route.duration(), 6_000 + 5_998)
    assert_equal(route.service_duration(), 6_000 + 1_140)
    assert_equal(route.time_warp(), 0)


def test_route_centroid(ok_small):
    """
    Tests that each route's center point is the center point of all clients
    visited by that route.
    """
    x = np.array([ok_small.location(idx).x for idx in range(5)])
    y = np.array([ok_small.location(idx).y for idx in range(5)])

    routes = [
        Route(ok_small, [1, 2], 0),
        Route(ok_small, [3], 0),
        Route(ok_small, [4], 0),
    ]

    for route in routes:
        x_center, y_center = route.centroid()
        assert_allclose(x_center, x[route].mean())
        assert_allclose(y_center, y[route].mean())


def test_route_can_be_pickled(rc208):
    """
    Tests that individual routes can be serialised and unserialised.
    """
    rng = RandomNumberGenerator(seed=2)
    sol = Solution.make_random(rc208, rng)

    for before_pickle in sol.routes():
        bytes = pickle.dumps(before_pickle)
        after_pickle = pickle.loads(bytes)

        assert_equal(after_pickle, before_pickle)


@pytest.mark.parametrize(
    ("tw_early", "start_late", "tw_late", "expected"),
    [
        (0, 0, 0, 20_277),  # cannot be back at the depot before 20'277
        (0, 10_000, 20_000, 277),  # larger shift window decreases time warp
        (0, 20_000, 20_000, 277),  # latest start does not affect time warp
        (0, 20_277, 20_277, 0),  # and in this case there is no more time warp
        (15_000, 15_000, 20_000, 1_221),  # minimum route duration is 6'221
        (10_000, 20_000, 20_000, 277),  # before earliest possible return
    ],
)
def test_route_shift_duration(
    ok_small,
    tw_early: int,
    start_late: int,
    tw_late: int,
    expected: int,
):
    """
    Tests that Route computes time warp due to shift durations correctly on a
    simple, two-client route.
    """
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(
                2,
                capacity=[10],
                tw_early=tw_early,
                tw_late=tw_late,
                start_late=start_late,
            )
        ]
    )

    # Overall route duration is, at the bare minimum, dist(0, 1) + dist(1, 2)
    # + dist(2, 0) + serv(1) + serv(2). That's 1'544 + 1'992 + 1'965 + 360
    # + 360 = 6'221. We cannot service client 1 before 15'600, and it takes
    # 1'544 to get there from the depot, so we leave at 14'056. Thus, the
    # earliest completion time is 14'056 + 6'221 = 20'277.
    route = Route(data, [1, 2], vehicle_type=0)
    assert_equal(route.time_warp(), expected)


def test_distance_duration_cost_calculations(ok_small):
    """
    Tests route-level distance and duration cost calculations.
    """
    vehicle_types = [
        VehicleType(capacity=[10], unit_distance_cost=5, unit_duration_cost=1),
        VehicleType(capacity=[10], unit_distance_cost=1, unit_duration_cost=5),
    ]
    data = ok_small.replace(vehicle_types=vehicle_types)

    routes = [Route(data, [1, 2], 0), Route(data, [3, 4], 1)]
    assert_equal(routes[0].distance_cost(), 5 * routes[0].distance())
    assert_equal(routes[0].duration_cost(), 1 * routes[0].duration())
    assert_equal(routes[1].distance_cost(), 1 * routes[1].distance())
    assert_equal(routes[1].duration_cost(), 5 * routes[1].duration())


def test_start_end_depot_not_same_on_empty_route(ok_small_multi_depot):
    """
    Tests that empty routes correctly evaluate distance and duration travelled
    between depots, even though there are no actual clients on the route.
    """
    vehicle_type = VehicleType(3, [10], start_depot=0, end_depot=1)
    data = ok_small_multi_depot.replace(vehicle_types=[vehicle_type])

    route = Route(data, [], vehicle_type=0)

    assert_equal(route.start_depot(), 0)
    assert_equal(route.end_depot(), 1)

    dist_mat = data.distance_matrix(0)
    assert_equal(route.distance(), dist_mat[0, 1])

    dur_mat = data.duration_matrix(0)
    assert_equal(route.duration(), dur_mat[0, 1])


def test_bug_start_time_before_release_time():
    """
    Tests that the bug identified in https://github.com/PyVRP/PyVRP/issues/633
    has been corrected.
    """
    mat = [[0, 10], [10, 0]]
    data = ProblemData(
        clients=[Client(1, 1, release_time=5)],
        depots=[Depot(0, 0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[mat],
        duration_matrices=[mat],
    )

    route = Route(data, [1], 0)
    assert_(route.is_feasible())
    assert_equal(route.start_time(), 5)
    assert_equal(route.end_time(), 25)


@pytest.mark.parametrize("visits", [[1, 2, 3, 4], [2, 1, 3, 4]])
def test_route_schedule(ok_small, visits: list[int]):
    """
    Tests that the route's schedule returns correct statistics for various
    routes.
    """
    route = Route(ok_small, visits, 0)
    schedule = route.schedule()

    for client, visit in zip(route, schedule):
        client_data: Client = ok_small.location(client)
        assert_equal(visit.service_duration, client_data.service_duration)
        assert_equal(
            visit.service_duration,
            visit.end_service - visit.start_service,
        )

    service_duration = sum(visit.service_duration for visit in schedule)
    assert_equal(service_duration, route.service_duration())

    wait_duration = sum(visit.wait_duration for visit in schedule)
    assert_equal(wait_duration, route.wait_duration())

    time_warp = sum(visit.time_warp for visit in schedule)
    assert_equal(time_warp, route.time_warp())


def test_route_schedule_wait_duration():
    """
    Tests that the route's schedule returns correct wait duration statistics.
    """
    data = read("data/OkSmallWaitTime.txt")
    route = Route(data, [2, 4], 0)
    schedule = route.schedule()

    # All wait duration is incurred at the last stop.
    assert_equal(schedule[-1].wait_duration, 1_550)
    assert_equal(route.wait_duration(), 1_550)

    wait_duration = sum(visit.wait_duration for visit in schedule)
    assert_equal(wait_duration, route.wait_duration())


def test_initial_load_calculation(ok_small):
    """
    Tests that loads are calculated correctly when there's an initial load
    present on the vehicle.
    """
    # Route load and vehicle capacity are both 10, so there should not be any
    # excess load.
    orig_route = Route(ok_small, [1, 2], 0)
    assert_equal(orig_route.excess_load(), [0])
    assert_(not orig_route.has_excess_load())

    # Modify data instance to have vehicle start with 5 initial load.
    veh_type = ok_small.vehicle_type(0)
    new_type = veh_type.replace(initial_load=[5])
    new_data = ok_small.replace(vehicle_types=[new_type])

    # Now there's 5 initial load, and 10 route load, with a vehicle capacity of
    # 10. This means there is now 5 excess load.
    new_route = Route(new_data, [1, 2], 0)
    assert_equal(new_route.excess_load(), [5])
    assert_(new_route.has_excess_load())

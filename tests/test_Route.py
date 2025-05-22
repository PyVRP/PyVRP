import pickle
from itertools import pairwise

import numpy as np
import pytest
from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

from pyvrp import (
    Client,
    Depot,
    ProblemData,
    RandomNumberGenerator,
    Route,
    Solution,
    Trip,
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


def test_access_multiple_trips(ok_small_multiple_trips):
    """
    Tests that accessing the route's clients via visits(), iteration, or the
    underlying trips works correctly for a multi-trip instance.
    """
    data = ok_small_multiple_trips
    trips = [Trip(data, [1, 2], 0), Trip(data, [3], 0)]
    route = Route(data, trips, 0)

    assert_equal(route.visits(), [1, 2, 3])
    assert_equal([client for client in route], [1, 2, 3])

    assert_equal(trips[0].visits(), [1, 2])
    assert_equal([client for client in trips[0]], [1, 2])

    assert_equal(trips[1].visits(), [3])
    assert_equal([client for client in trips[1]], [3])


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
    assert_equal(len(schedule), len(route) + 2)  # schedule includes depots

    for visit in schedule:
        data = ok_small.location(visit.location)
        service = getattr(data, "service_duration", 0)  # only for clients
        assert_equal(visit.service_duration, service)
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

    # All wait duration is incurred at the last client stop.
    assert_equal(schedule[-2].wait_duration, 1_550)
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


def test_bug_initial_load_multiple_trips(ok_small_multiple_trips):
    """
    Tests that a bug where initial load was evaluated at each trip has been
    corrected: initial load is a route-level property, not trip-level.
    """
    veh_type = ok_small_multiple_trips.vehicle_type(0)
    new_type = veh_type.replace(initial_load=[5])
    data = ok_small_multiple_trips.replace(vehicle_types=[new_type])

    trip1 = Trip(data, [1, 2], 0)
    trip2 = Trip(data, [3, 4], 0)

    # Both trips are load feasible.
    assert_(not trip1.has_excess_load())
    assert_(not trip2.has_excess_load())

    # But the route is not because the first trip has a load of 10, in addition
    # to the initial load of 5: 15 > 10.
    route = Route(data, [trip1, trip2], 0)
    assert_equal(route.excess_load(), [5])


@pytest.mark.parametrize(("start_depot", "end_depot"), [(0, 1), (1, 0)])
def test_raises_if_route_does_not_start_and_end_at_vehicle_start_end_depots(
    ok_small_multi_depot, start_depot: int, end_depot: int
):
    """
    Tests that the route constructor raises when the route implied by the
    sequence of trips does not start at the vehicle type's start_depot, or end
    at the end_depot.
    """
    old_veh_type = ok_small_multi_depot.vehicle_type(0)
    veh_type = old_veh_type.replace(reload_depots=[0, 1], max_reloads=2)
    data = ok_small_multi_depot.replace(vehicle_types=[veh_type])

    trip1 = Trip(data, [2], 0, start_depot, 1)
    trip2 = Trip(data, [3], 0, 1, end_depot)

    with assert_raises(ValueError):
        Route(data, [trip1, trip2], 0)


def test_raises_inconsistent_vehicle_type(ok_small_two_profiles):
    """
    Tests that the route constructor raises when trips do not share the route's
    vehicle type.
    """
    trip = Trip(ok_small_two_profiles, [], 1)
    assert_equal(trip.vehicle_type(), 1)

    with assert_raises(ValueError):
        Route(ok_small_two_profiles, [trip], 0)


def test_raises_consecutive_trips_different_depots(ok_small_multi_depot):
    """
    Tests that the route constructor raises when consecutive trips disagree on
    their start and end depots.
    """
    old_veh_type = ok_small_multi_depot.vehicle_type(0)
    veh_type = old_veh_type.replace(reload_depots=[0, 1], max_reloads=2)
    data = ok_small_multi_depot.replace(vehicle_types=[veh_type])

    trip1 = Trip(data, [2], 0, 0, 1)
    trip2 = Trip(data, [3], 0, 0, 0)
    assert_equal(trip1.end_depot(), 1)
    assert_equal(trip2.start_depot(), 0)

    with assert_raises(ValueError):
        Route(data, [trip1, trip2], 0)


def test_raises_multiple_trips_without_reload_depots(ok_small):
    """
    Tests that the route constructor raises when there is more than one trip,
    yet the vehicle type does not support reloading.
    """
    assert_equal(len(ok_small.vehicle_type(0).reload_depots), 0)

    trips = [Trip(ok_small, [1, 2], 0), Trip(ok_small, [3], 0)]
    with assert_raises(ValueError):
        Route(ok_small, trips, 0)


def test_raises_vehicle_max_reloads(ok_small_multiple_trips):
    """
    Tests that the route constructor raises when there are more reloads than
    the vehicle supports.
    """
    veh_type = ok_small_multiple_trips.vehicle_type(0)
    assert_equal(veh_type.max_reloads, 1)

    trip1 = Trip(ok_small_multiple_trips, [1], 0)
    trip2 = Trip(ok_small_multiple_trips, [2], 0)
    trip3 = Trip(ok_small_multiple_trips, [3], 0)

    with assert_raises(ValueError):
        Route(ok_small_multiple_trips, [trip1, trip2, trip3], 0)


def test_str(ok_small_multiple_trips):
    """
    Tests that a route's string representation correctly uses a | to separate
    multiple trips.
    """
    data = ok_small_multiple_trips
    trips = [Trip(data, [1, 2], 0), Trip(data, [3], 0)]

    route1 = Route(data, trips, 0)
    assert_equal(str(route1), "1 2 | 3")

    route2 = Route(data, [trips[0]], 0)
    assert_equal(str(route2), "1 2")

    route3 = Route(data, [trips[1]], 0)
    assert_equal(str(route3), "3")

    route4 = Route(data, [], 0)
    assert_equal(str(route4), "")


def test_statistics_with_small_multi_trip_example(ok_small_multiple_trips):
    """
    Tests some statistics calculations on a small multi-trip example.
    """
    # Regular route, executed in a single trip.
    route1 = Route(ok_small_multiple_trips, [1, 2, 3, 4], 0)

    # Same visits but executed over two trips.
    trip1 = Trip(ok_small_multiple_trips, [1, 2], 0)
    trip2 = Trip(ok_small_multiple_trips, [3, 4], 0)
    route2 = Route(ok_small_multiple_trips, [trip1, trip2], 0)

    assert_equal(route2.visits(), route1.visits())
    assert_equal(len(route2), len(route1))
    assert_equal(route1.num_trips(), 1)
    assert_equal(route2.num_trips(), 2)

    # Route structure and general statistics.
    assert_equal(route2.prizes(), route1.prizes())
    assert_allclose(route2.centroid(), route1.centroid())
    assert_equal(route2.start_depot(), route1.start_depot())
    assert_equal(route2.end_depot(), route1.end_depot())

    # First route has excess load, second does not.
    assert_equal(route1.excess_load(), [8])
    assert_equal(route2.excess_load(), [0])

    # First route takes 7'950, but the second route takes longer, because it
    # has to reload at the depot. The difference is exactly the difference in
    # arc travel time.
    durs = ok_small_multiple_trips.duration_matrix(0)
    diff = durs[2, 0] + durs[0, 3] - durs[2, 3]

    assert_equal(route1.duration(), 7_950)
    assert_equal(route2.duration(), route1.duration() + diff)
    assert_equal(route2.wait_duration(), route1.wait_duration())
    assert_equal(route2.service_duration(), route1.service_duration())


def test_schedule_multi_trip_example(ok_small_multiple_trips):
    """
    Tests that schedule() includes the depot visits, which is particularly
    important when a route consists of multiple trips.
    """
    trip1 = Trip(ok_small_multiple_trips, [1, 2], 0)
    trip2 = Trip(ok_small_multiple_trips, [3, 4], 0)
    route = Route(ok_small_multiple_trips, [trip1, trip2], 0)

    schedule = route.schedule()
    locations = [visit.location for visit in schedule]
    assert_equal(locations, [0, 1, 2, 0, 3, 4, 0])


def test_index_multiple_trips(ok_small_multiple_trips):
    """
    Tests that direct indexing a route object with multiple trips finds the
    correct client associated with each index.
    """
    trip1 = Trip(ok_small_multiple_trips, [1], 0)
    trip2 = Trip(ok_small_multiple_trips, [3], 0)

    route = Route(ok_small_multiple_trips, [trip1, trip2], 0)
    assert_equal(route[0], 1)
    assert_equal(route[-2], 1)

    assert_equal(route[1], 3)
    assert_equal(route[-1], 3)

    with assert_raises(IndexError):
        route[2]


def test_iter_empty_trips(ok_small_multiple_trips):
    """
    Tests that iterating a route also gracefully handles empty trips.
    """
    veh_type = ok_small_multiple_trips.vehicle_type(0).replace(max_reloads=2)
    data = ok_small_multiple_trips.replace(vehicle_types=[veh_type])

    trip1 = Trip(data, [1, 2], 0)
    trip2 = Trip(data, [], 0)
    trip3 = Trip(data, [3, 4], 0)

    route = Route(data, [trip1, trip2, trip3], 0)
    assert_equal(str(route), "1 2 |  | 3 4")
    assert_equal(route.num_trips(), 3)
    assert_equal(list(route), [1, 2, 3, 4])


def test_small_example_from_cattaruzza_paper():
    """
    Tests a small multi-trip VRP example. The data are from [1]_, corresponding
    to their Figure 1.

    References
    ----------
    .. [1] D. Cattaruzza, N. Absi, and D. Feillet (2016). The Multi-Trip
           Vehicle Routing Problem with Time Windows and Release Dates.
           *Transportation Science* 50(2): 676-693.
           https://doi.org/10.1287/trsc.2015.0608.
    """
    # The paper has 20 service duration at the depot. We do not have this field
    # so we instead add the 20 extra time to the outgoing depot arcs.
    depot = Depot(0, 0, tw_early=0, tw_late=200)
    clients = [
        # Figure 1 details release times for some clients. But release times
        # are not actually binding in the example, so they are not needed.
        Client(0, 0, tw_early=100, tw_late=120, service_duration=5),
        Client(0, 0, tw_early=50, tw_late=75, service_duration=5),
        Client(0, 0, tw_early=50, tw_late=75, service_duration=5),
        Client(0, 0, tw_early=50, tw_late=100, service_duration=5),
        Client(0, 0, tw_early=50, tw_late=100, service_duration=5),
    ]

    matrix = [
        [0, 25, 35, 40, 30, 35],  # +20 for depot service duration
        [5, 0, 20, 20, 15, 15],
        [15, 20, 0, 40, 20, 30],
        [20, 20, 40, 0, 30, 10],
        [10, 15, 20, 30, 0, 20],
        [15, 15, 30, 10, 20, 0],
    ]

    data = ProblemData(
        clients=clients,
        depots=[depot],
        vehicle_types=[VehicleType(reload_depots=[0])],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
    )

    trip1 = Trip(data, [5, 3], 0)
    trip2 = Trip(data, [1], 0)
    trip3 = Trip(data, [4], 0)
    trip4 = Trip(data, [2], 0)
    route = Route(data, [trip1, trip2, trip3, trip4], 0)

    # These values were verified from the paper, and where needed calculated
    # manually. Note that the paper setting requires the first trip to start
    # immediately, which results in 15 wait duration. We do not require this,
    # instead starting a little later to avoid the wait duration. The other
    # quantities reported below match the paper.
    assert_equal(route.start_time(), 15)
    assert_equal(route.end_time(), 95)
    assert_equal(route.time_warp(), 75 + 55)  # two time warp violations
    assert_equal(route.slack(), 0)  # there is time warp, so no slack
    assert_equal(route.service_duration(), 25)  # at clients
    assert_equal(route.travel_duration(), 185)  # incl. 80 'service' at depots


def test_multi_trip_with_release_times():
    """
    Test a small example with multiple trips and (binding) release times.
    """
    matrix = [
        [0, 30, 20, 40],
        [0, 0, 10, 0],
        [5, 0, 0, 0],
        [10, 0, 0, 0],
    ]

    data = ProblemData(
        clients=[
            Client(0, 0, tw_early=60, tw_late=100, release_time=40),
            Client(0, 0, tw_early=70, tw_late=90, release_time=50),
            Client(0, 0, tw_early=80, tw_late=150, release_time=100),
        ],
        depots=[Depot(0, 0)],
        vehicle_types=[VehicleType(reload_depots=[0])],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
    )

    trip1 = Trip(data, [1, 2], 0)
    trip2 = Trip(data, [3], 0)

    assert_equal(trip1.release_time(), 50)
    assert_equal(trip2.release_time(), 100)

    # Route should have a release time corresponding to its first trip.
    route = Route(data, [trip1, trip2], 0)
    assert_equal(route.release_time(), trip1.release_time())

    # Travel is explained better below.
    assert_equal(trip1.travel_duration(), 45)
    assert_equal(trip2.travel_duration(), 50)
    assert_equal(route.travel_duration(), 95)

    # No service.
    assert_equal(trip1.service_duration(), 0)
    assert_equal(trip2.service_duration(), 0)
    assert_equal(route.service_duration(), 0)

    # Some route-level statistics. We start at 50, the release time for the
    # first trip. Then we drive to client 1 and arrive at 80. We do service,
    # and drive to 2, where we arrive at 90. Again, service and drive to 0,
    # where we arrive at 95. We then wait until 100, the release time for the
    # second trip. We then drive to 3, where we arrive at 140. We service, and
    # drive back to the depot, where we arrive at 150. We finish at 150.
    assert_equal(route.start_time(), 50)
    assert_equal(route.duration(), 100)
    assert_equal(route.end_time(), 150)
    assert_equal(route.wait_duration(), 5)  # waiting for release time
    assert_equal(route.time_warp(), 0)
    assert_equal(route.slack(), 0)  # the schedule is tight

    schedule = route.schedule()

    assert_equal(schedule[0].start_service, 50)
    assert_equal(schedule[0].end_service, 50)

    assert_equal(schedule[3].start_service, 100)
    assert_equal(schedule[3].end_service, 100)
    assert_equal(schedule[3].wait_duration, 5)

    assert_equal(schedule[-1].start_service, 150)
    assert_equal(schedule[-1].end_service, 150)


def test_multi_trip_initial_load(ok_small_multiple_trips):
    """
    Tests that initial load is correctly calculated in a multi-trip setting.
    """
    old_type = ok_small_multiple_trips.vehicle_type(0)
    new_type = old_type.replace(initial_load=[5])
    data = ok_small_multiple_trips.replace(vehicle_types=[new_type])

    trip1 = Trip(data, [1, 2], 0)
    trip2 = Trip(data, [3, 4], 0)
    route = Route(data, [trip1, trip2], 0)

    # The trips themselves are fine, but the vehicle has initial load, and that
    # causes the first trip - which has 10 delivery load already - to exceed
    # the vehicle's capacity.
    assert_equal(trip1.excess_load(), [0])
    assert_equal(trip2.excess_load(), [0])
    assert_equal(route.excess_load(), [5])


@pytest.mark.parametrize(
    "trips",
    [
        [[61, 64, 49, 55, 54, 53], [70, 11, 10, 8]],
        [[5, 2, 1, 7, 3, 4], [18, 19, 16, 14, 12]],
        [[20, 22, 24, 27, 30, 29, 6], [68, 57, 40, 44, 46], [15, 17, 13, 9]],
    ],
)
def test_multi_trip_release_time_routes(mtvrptw_release_times, trips):
    """
    Tests a few routes from a solution to this multi-trip instance with release
    times. These routes were part of a best-found solution after a 5min run, so
    they should be feasible. Furthermore, the release times of each trip should
    obviously be non-decreasing.
    """
    trips = [Trip(mtvrptw_release_times, visits, 0) for visits in trips]
    route = Route(mtvrptw_release_times, trips, 0)
    assert_(route.is_feasible())

    for trip1, trip2 in pairwise(trips):
        assert_(trip1.release_time() <= trip2.release_time())

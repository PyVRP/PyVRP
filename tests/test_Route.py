import pickle

import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import (
    Activity,
    Client,
    Depot,
    Location,
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
    depot indices.
    """
    routes = [
        Route(ok_small_multi_depot, [0], 0),
        Route(ok_small_multi_depot, [1, 2], 1),
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
    routes = [Route(ok_small, [0, 2], 0), Route(ok_small, [1, 3], 0)]

    # There's no excess load, so all excess load should be zero.
    assert_equal(routes[0].excess_load(), [0])
    assert_equal(routes[1].excess_load(), [0])

    # Total route delivery demand (and pickups, which are all zero for this
    # instance).
    deliveries = [client.delivery[0] for client in ok_small.clients()]
    assert_equal(routes[0].delivery(), [deliveries[0] + deliveries[2]])
    assert_equal(routes[1].delivery(), [deliveries[1] + deliveries[3]])

    assert_equal(routes[0].pickup(), [0])
    assert_equal(routes[1].pickup(), [0])

    # The first route is not feasible due to time warp, but the second one is.
    # See also the tests below.
    assert_(not routes[0].is_feasible())
    assert_(routes[1].is_feasible())

    # Total service duration.
    services = [client.service_duration for client in ok_small.clients()]
    assert_equal(routes[0].service_duration(), services[0] + services[2])
    assert_equal(routes[1].service_duration(), services[1] + services[3])


def test_access_multiple_trips(ok_small_multiple_trips):
    """
    Tests that accessing the route's schedule via schedule() or iteration works
    correctly for a multi-trip instance.
    """
    data = ok_small_multiple_trips
    activities = [Activity(des) for des in ["C0", "C1", "D0", "C2"]]
    route = Route(data, activities, vehicle_type=0)

    incl_depots = [Activity("D0"), *activities, Activity("D0")]
    assert_equal(route.schedule(), incl_depots)
    assert_equal([activity for activity in route], incl_depots)


def test_route_time_warp_calculations(ok_small):
    """
    Tests route time warp calculations.
    """
    routes = [Route(ok_small, [0, 2], 0), Route(ok_small, [1, 3], 0)]

    # There is time warp on the first route: duration(D0, C0) = 1'544, so we
    # arrive at C0 before its opening window of 15'600. Service (360) thus
    # starts at 15'600, and completes at 15'600 + 360. Then we drive for
    # duration(C0, C2) = 1'427, where we arrive after 15'300 (its closing time
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
    routes = [Route(data, [0, 2], 0), Route(data, [1, 3], 0)]

    # The time window of C1 closes at 15'000. After service and travel, we then
    # arrive at C3 before its time window opens, so we have to wait for:
    #   twEarly(C3) - duration(C1, C3) - serv(C1) - twLate(C1)
    #     = 18'000 - 1'090 - 360 - 15'000
    #     = 1'550.
    assert_equal(routes[1].wait_duration(), 1_550)

    # Since there is waiting time, there is no slack in the schedule. We should
    # thus start as late as possible, at:
    #   twLate(C1) - duration(D0, C1)
    #     = 15'000 - 1'944
    #     = 13'056.
    assert_equal(routes[1].slack(), 0)
    assert_equal(routes[1].start_time(), 13_056)

    # So far we have tested a route that had wait duration, but not time warp.
    # The following route has both.
    route = Route(data, [0, 1, 3], 0)

    # This route has the same wait time as explained above. The time warp is
    # incurred earlier in the route, between C0 -> C1:
    #   twEarly(C0) + serv(C0) + duration(C0, C1) - twLate(C1)
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
    routes = [Route(ok_small, [0, 2], 0), Route(ok_small, [1, 3], 0)]

    # The first route has timewarp, so there is no slack in the schedule. We
    # should thus depart as soon as possible to arrive at the first client the
    # moment its time window opens.
    durations = ok_small.duration_matrix(profile=0)
    start_time = ok_small.client(0).tw_early - durations[0, 1]
    end_time = start_time + routes[0].duration() - routes[0].time_warp()

    assert_(routes[0].has_time_warp())
    assert_equal(routes[0].slack(), 0)
    assert_equal(routes[0].start_time(), start_time)
    assert_equal(routes[0].end_time(), end_time)

    # The second route has no time warp. The latest it can start is calculated
    # backwards from the closing of C3's time window:
    #   twLate(C3) - dur(C1, C3) - serv(C1) - dur(D0, C1)
    #     = 19'500 - 1'090 - 360 - 1'944
    #     = 16'106.
    #
    # Because C3 has a large time window, the earliest this route can start is
    # determined completely by C1: we should not arrive before its time window,
    # because that would incur needless waiting. We should thus not depart
    # before:
    #   twEarly(C1) - dur(D0, C1)
    #     = 12'000 - 1'944
    #     = 10'056.
    assert_(not routes[1].has_time_warp())
    assert_equal(routes[1].wait_duration(), 0)
    assert_equal(routes[1].start_time(), 10_056)
    assert_equal(routes[1].slack(), 16_106 - 10_056)

    # The overall route duration is given by:
    #   dur(D0, C1) + serv(C1) + dur(C1, C3) + serv(C3) + dur(C3, D0)
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
    routes = [Route(data, [0, 2], 0), Route(data, [1, 3], 0)]

    # The client release times are 20'000, 5'000, 5'000 and 1'000. So the first
    # route has a release time of max(20'000, 5'000) = 20'000, and the second
    # has a release time of max(5'000, 1'000) = 5'000.
    assert_equal(routes[0].release_time(), 20_000)
    assert_equal(routes[1].release_time(), 5_000)

    # Second route is feasible, so should have start time not smaller than
    # release time.
    assert_(not routes[1].has_time_warp())
    assert_(routes[1].start_time() > routes[1].release_time())


def test_release_time_and_shift_duration():
    """
    Tests the interaction of release times and shift duration constraints. In
    particular, we verify that the shift duration applies to the time after the
    vehicle starts their route, and that the release time only shifts that
    starting moment - it does not affect the overall maximum duration.
    """
    ok_small = read("data/OkSmallReleaseTimes.txt")
    vehicle_type = VehicleType(3, [10], shift_duration=5_000)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    # This route has a release time of 5000, but should not start until much
    # later because of the time windows. The vehicle's shift duration is also
    # 5000, but this is violated by 998 units of time. The route is otherwise
    # feasible, so there is exactly 998 time warp.
    route = Route(data, [1, 2, 3], 0)
    assert_equal(route.release_time(), 5_000)
    assert_equal(route.start_time(), 10_056)
    assert_equal(route.end_time(), 15_056)
    assert_equal(route.duration(), 5_998)
    assert_equal(route.service_duration(), 1_140)
    assert_equal(route.time_warp(), 998)


def test_release_time_and_service_duration():
    """
    Tests the interaction between release times and depot service duration, and
    checks that service at the depot happens after the tasks are released. See
    also the ``test_release_time_and_shift_duration`` test.
    """
    ok_small = read("data/OkSmallReleaseTimes.txt")
    depot = Depot(0, service_duration=6_000)
    data = ok_small.replace(depots=[depot])

    # This route has a release time of 5000, but we want to start much later
    # anyway because of the time windows. That's not possible, however, because
    # of the depot service duration of 6000. The overall route duration is 5998
    # plus the 6000, and there is no time warp.
    route = Route(data, [1, 2, 3], 0)
    assert_equal(route.release_time(), 5_000)
    assert_equal(route.start_time(), 5_000)
    assert_equal(route.duration(), 6_000 + 5_998)
    assert_equal(route.service_duration(), 6_000 + 1_140)
    assert_equal(route.time_warp(), 0)


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

    # Overall route duration is, at the bare minimum,
    #   dist(D0, C0) + dist(C0, C1) + dist(C1, D0) + serv(C0) + serv(C1)
    #       = 1'544 + 1'992 + 1'965 + 360 + 360
    #       = 6'221.
    # We cannot service C0 before 15'600, and it takes 1'544 to get there from
    # the depot, so we leave at 14'056. Thus, the earliest completion time is
    # 14'056 + 6'221 = 20'277.
    route = Route(data, [0, 1], vehicle_type=0)
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

    routes = [Route(data, [0, 1], 0), Route(data, [2, 3], 1)]
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
        locations=[Location(0, 0), Location(1, 1)],
        clients=[Client(1, release_time=5)],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[mat],
        duration_matrices=[mat],
    )

    route = Route(data, [0], 0)
    assert_(route.is_feasible())
    assert_equal(route.start_time(), 5)
    assert_equal(route.end_time(), 25)


@pytest.mark.parametrize("visits", [[0, 1, 2, 3], [1, 0, 2, 3]])
def test_route_schedule(ok_small, visits: list[int]):
    """
    Tests that the route's schedule returns correct statistics for various
    routes.
    """
    route = Route(ok_small, visits, 0)
    schedule = route.schedule()
    assert_equal(len(schedule), len(route))

    for activity in schedule:
        if activity.is_depot():
            data = ok_small.depot(activity.idx)
        else:
            data = ok_small.client(activity.idx)

        service = data.service_duration
        duration = activity.duration
        assert_equal(duration, service)
        assert_equal(duration, activity.end_time - activity.start_time)

    service_duration = sum(activity.duration for activity in schedule)
    assert_equal(service_duration, route.service_duration())

    wait_duration = sum(activity.wait_duration for activity in schedule)
    assert_equal(wait_duration, route.wait_duration())

    time_warp = sum(activity.time_warp for activity in schedule)
    assert_equal(time_warp, route.time_warp())


def test_route_schedule_wait_duration():
    """
    Tests that the route's schedule returns correct wait duration statistics.
    """
    data = read("data/OkSmallWaitTime.txt")
    route = Route(data, [1, 3], 0)
    schedule = route.schedule()

    # All wait duration is incurred at the last client stop.
    assert_equal(schedule[-2].wait_duration, 1_550)
    assert_equal(route.wait_duration(), 1_550)

    wait_duration = sum(activity.wait_duration for activity in schedule)
    assert_equal(wait_duration, route.wait_duration())


def test_initial_load_calculation(ok_small):
    """
    Tests that loads are calculated correctly when there's an initial load
    present on the vehicle.
    """
    # Route load and vehicle capacity are both 10, so there should not be any
    # excess load.
    orig_route = Route(ok_small, [0, 1], 0)
    assert_equal(orig_route.excess_load(), [0])
    assert_(not orig_route.has_excess_load())

    # Modify data instance to have vehicle start with 5 initial load.
    veh_type = ok_small.vehicle_type(0)
    new_type = veh_type.replace(initial_load=[5])
    new_data = ok_small.replace(vehicle_types=[new_type])

    # Now there's 5 initial load, and 10 route load, with a vehicle capacity of
    # 10. This means there is now 5 excess load.
    new_route = Route(new_data, [0, 1], 0)
    assert_equal(new_route.excess_load(), [5])
    assert_(new_route.has_excess_load())


def test_raises_multiple_trips_without_reload_depots(ok_small):
    """
    Tests that the route constructor raises when there is more than one trip,
    yet the vehicle type does not support reloading.
    """
    assert_equal(len(ok_small.vehicle_type(0).reload_depots), 0)

    activities = map(Activity, ["C1", "C2", "D0", "C3"])
    with assert_raises(ValueError):
        Route(ok_small, activities, 0)


def test_raises_vehicle_max_reloads(ok_small_multiple_trips):
    """
    Tests that the route constructor raises when there are more reloads than
    the vehicle supports.
    """
    veh_type = ok_small_multiple_trips.vehicle_type(0)
    assert_equal(veh_type.max_reloads, 1)

    activities = map(Activity, ["C1", "D0", "D0", "C3"])
    with assert_raises(ValueError):
        Route(ok_small_multiple_trips, activities, 0)


def test_str(ok_small_multiple_trips):
    """
    Tests that a route's string representation correctly uses a | to separate
    multiple trips.
    """
    data = ok_small_multiple_trips
    activities = [Activity(des) for des in ["C0", "C1", "D0", "C2"]]

    route1 = Route(data, activities, 0)
    assert_equal(str(route1), "C0 C1 | C2")

    route2 = Route(data, activities[:2], 0)
    assert_equal(str(route2), "C0 C1")

    route3 = Route(data, [activities[-1]], 0)
    assert_equal(str(route3), "C2")

    route4 = Route(data, [], 0)
    assert_equal(str(route4), "")


def test_statistics_with_small_multi_trip_example(ok_small_multiple_trips):
    """
    Tests some statistics calculations on a small multi-trip example.
    """
    # Regular route, executed in a single trip.
    route1 = Route(ok_small_multiple_trips, [0, 1, 2, 3], 0)

    # Same visits but executed over two trips.
    activities = map(Activity, ["C0", "C1", "D0", "C2", "C3"])
    route2 = Route(ok_small_multiple_trips, activities, 0)

    assert_equal(route1.num_trips(), 1)
    assert_equal(route2.num_trips(), 2)

    # Route structure and general statistics.
    assert_equal(route2.prizes(), route1.prizes())
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


def test_index_multiple_trips(ok_small_multiple_trips):
    """
    Tests that direct indexing a route object with multiple trips finds the
    correct client associated with each index.
    """
    activities = [Activity(des) for des in ["C0", "D0", "C2"]]
    route = Route(ok_small_multiple_trips, activities, 0)
    assert_equal(route[0], Activity("D0"))  # start
    assert_equal(route[-3], Activity("D0"))  # reload

    assert_equal(route[1], Activity("C0"))
    assert_equal(route[-2], Activity("C2"))

    with assert_raises(IndexError):
        route[5]


def test_iter_empty_trips(ok_small_multiple_trips):
    """
    Tests that iterating a route also gracefully handles empty trips.
    """
    veh_type = ok_small_multiple_trips.vehicle_type(0).replace(max_reloads=2)
    data = ok_small_multiple_trips.replace(vehicle_types=[veh_type])

    activities = map(Activity, ["C0", "C1", "D0", "D0", "C2", "C3"])
    route = Route(data, activities, 0)
    assert_equal(str(route), "C0 C1 | | C2 C3")
    assert_equal(route.num_trips(), 3)


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
    depot = Depot(0, tw_early=0, tw_late=200, service_duration=20)
    clients = [
        # Figure 1 details release times for some clients. But release times
        # are not actually binding in the example, so they are not needed.
        Client(1, tw_early=100, tw_late=120, service_duration=5),
        Client(2, tw_early=50, tw_late=75, service_duration=5),
        Client(3, tw_early=50, tw_late=75, service_duration=5),
        Client(4, tw_early=50, tw_late=100, service_duration=5),
        Client(5, tw_early=50, tw_late=100, service_duration=5),
    ]

    matrix = [
        [0, 5, 15, 20, 10, 15],
        [5, 0, 20, 20, 15, 15],
        [15, 20, 0, 40, 20, 30],
        [20, 20, 40, 0, 30, 10],
        [10, 15, 20, 30, 0, 20],
        [15, 15, 30, 10, 20, 0],
    ]

    data = ProblemData(
        locations=[Location(0, 0) for _ in range(6)],
        clients=clients,
        depots=[depot],
        vehicle_types=[VehicleType(reload_depots=[0])],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
    )

    acts = map(Activity, ["C4", "C2", "D0", "C0", "D0", "C3", "D0", "C1"])
    route = Route(data, acts, 0)

    # These values were verified from the paper, and where needed calculated
    # manually. Note that the paper setting requires the first trip to start
    # immediately, which results in 15 wait duration. We do not require this,
    # instead starting a little later to avoid the wait duration. The other
    # quantities reported below match the paper.
    assert_equal(route.start_time(), 15)
    assert_equal(route.end_time(), 95)
    assert_equal(route.time_warp(), 75 + 55)  # two time warp violations
    assert_equal(route.slack(), 0)  # there is time warp, so no slack
    assert_equal(route.service_duration(), 25 + 80)  # at clients and depots
    assert_equal(route.travel_duration(), 105)

    # Numbers from the paper. Trip 3 starts at 125, 4 at 115, and the route
    # ends at the end depot at 95.
    schedule = route.schedule()
    assert_equal(schedule[5].start_time, 125)  # trip 3
    assert_equal(schedule[5].duration, 20)

    assert_equal(schedule[7].start_time, 115)  # trip 4
    assert_equal(schedule[7].duration, 20)

    assert_equal(schedule[9].start_time, 95)  # end depot
    assert_equal(schedule[9].duration, 0)


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
        locations=[Location(0, 0) for _ in range(4)],
        clients=[
            Client(1, tw_early=60, tw_late=100, release_time=40),
            Client(2, tw_early=70, tw_late=90, release_time=50),
            Client(3, tw_early=80, tw_late=150, release_time=100),
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(reload_depots=[0])],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
    )

    # Route should have a release time corresponding to its first trip.
    route = Route(data, map(Activity, ["C0", "C1", "D0", "C2"]), 0)
    assert_equal(route.release_time(), 50)  # from C1, which is binding
    assert_equal(route.travel_duration(), 95)
    assert_equal(route.service_duration(), 0)

    # Some route-level statistics. We start at 50, the release time for the
    # first trip. Then we drive to C0 and arrive at 80. We do service, and
    # drive to C1, where we arrive at 90. Again, service and drive to D0, where
    # we arrive at 95. We then wait until 100, the release time for the
    # second trip. We then drive to C2, where we arrive at 140. We service, and
    # drive back to the depot, where we arrive at 150. We finish at 150.
    assert_equal(route.start_time(), 50)
    assert_equal(route.duration(), 100)
    assert_equal(route.end_time(), 150)
    assert_equal(route.wait_duration(), 5)  # waiting for release time
    assert_equal(route.time_warp(), 0)
    assert_equal(route.slack(), 0)  # the schedule is tight

    schedule = route.schedule()

    assert_equal(schedule[0].start_time, 50)
    assert_equal(schedule[0].end_time, 50)

    assert_equal(schedule[3].start_time, 100)
    assert_equal(schedule[3].end_time, 100)
    assert_equal(schedule[3].wait_duration, 5)

    assert_equal(schedule[-1].start_time, 150)
    assert_equal(schedule[-1].end_time, 150)


def test_multi_trip_initial_load(ok_small_multiple_trips):
    """
    Tests that initial load is correctly calculated in a multi-trip setting.
    """
    old_type = ok_small_multiple_trips.vehicle_type(0)
    new_type = old_type.replace(initial_load=[5])
    data = ok_small_multiple_trips.replace(vehicle_types=[new_type])

    # The trips themselves are fine, but the vehicle has initial load, and that
    # causes the first trip - which has 10 delivery load already from C0 and C1
    # - to exceed the vehicle's capacity.
    activities = map(Activity, ["C0", "C1", "D0", "C2", "C3"])
    route = Route(data, activities, 0)
    assert_equal(route.excess_load(), [5])


def test_bug_iterating_with_empty_last_trip(ok_small_multiple_trips):
    """
    Ensures that the bug identified in #812 stays fixed. Before the fix, this
    would trigger an assert because an empty trip would be indexed.
    """
    activities = map(Activity, ["C0", "C1", "D0"])
    route = Route(ok_small_multiple_trips, activities, 0)

    # D0 -> C0 -> C1 -> D0 -> D0.
    assert_equal([activity.idx for activity in route], [0, 0, 1, 0, 0])


def test_route_release_time_after_vehicle_start_late():
    """
    Tests that a route time warps back to the vehicle's latest start if that
    latest start is before the first trip's release time.
    """
    data = read("data/OkSmallReleaseTimes.txt")

    new_type = data.vehicle_type(0).replace(start_late=10_000)
    data = data.replace(vehicle_types=[new_type])
    route = Route(data, [2, 0], 0)

    # Route starts at 20_000 due to release time. We immediately time warp back
    # to 10_000 for the vehicle's latest start constraint. From there, we visit
    # 3 and 1 as regular, without accruing any additional time warp.
    assert_equal(route.start_time(), 20_000)
    assert_equal(route.release_time(), 20_000)
    assert_equal(route.time_warp(), 10_000)
    assert_equal(route.duration(), 7_686)

    # Sanity check that all time warp is also accounted for in the schedule.
    assert_equal(sum(v.time_warp for v in route.schedule()), route.time_warp())


@pytest.mark.parametrize("fixed_cost", (0, 10))
def test_fixed_vehicle_cost(ok_small, fixed_cost: int):
    """
    Tests the Route's fixed_vehicle_cost() method.
    """
    veh_type = ok_small.vehicle_type(0).replace(fixed_cost=fixed_cost)
    data = ok_small.replace(vehicle_types=[veh_type])

    route = Route(data, [], 0)
    assert_equal(route.fixed_vehicle_cost(), fixed_cost)


def test_raises_invalid_depot_or_client(ok_small_multiple_trips):
    """
    Tests that Route's constructor raises for invalid depot or client
    activities.
    """
    data = ok_small_multiple_trips

    assert_equal(data.num_depots, 1)
    with assert_raises(ValueError):  # D1 does not exist
        Route(data, [Activity("D1")], vehicle_type=0)

    assert_equal(data.num_clients, 4)
    with assert_raises(ValueError):  # C4 does not exist
        Route(data, [Activity("C4")], vehicle_type=0)

    # But D0 and C3 do exist, so this should be OK.
    Route(data, [Activity("C3"), Activity("D0")], vehicle_type=0)


def test_schedule_str(ok_small):
    """
    Tests ScheduledActivity's __str__ implementation.
    """
    route = Route(ok_small, [0, 1], 0)

    schedule = route.schedule()
    assert_equal(str(schedule[0]), "D0")
    assert_equal(str(schedule[1]), "C0")
    assert_equal(str(schedule[2]), "C1")
    assert_equal(str(schedule[3]), "D0")


def test_len(ok_small_multiple_trips):
    """
    Tests that Route counts all activities for its length, including depot
    visits. Also tests number of trips and clients.
    """
    data = ok_small_multiple_trips
    route = Route(data, [Activity("C0"), Activity("D0"), Activity("C1")], 0)
    assert_equal(len(route), 5)
    assert_equal(route.num_clients(), 2)
    assert_equal(route.num_depots(), 3)
    assert_equal(route.num_trips(), 2)


def test_schedule_trip_count(ok_small_multiple_trips):
    """
    Tests that the trip counter for scheduled activities starts at 0, and is
    incremented at every subsequent (reload) depot.
    """
    data = ok_small_multiple_trips
    route = Route(data, [Activity("C0"), Activity("D0")], 0)
    assert_equal(route[0].trip, 0)  # start depot
    assert_equal(route[1].trip, 0)  # client, so no increment
    assert_equal(route[2].trip, 1)  # reload depot, increment
    assert_equal(route[3].trip, 2)  # end depot, increment

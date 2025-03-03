import pytest
from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

from pyvrp import Route, Trip


@pytest.mark.parametrize(("start_idx", "end_idx"), [(1, 0), (0, 1)])
def test_trip_raises_for_invalid_depot_arguments(ok_small, start_idx, end_idx):
    """
    The constructor should raise when the start or end depot arguments point to
    non-existing depots.
    """
    assert_equal(ok_small.num_depots, 1)
    with assert_raises(ValueError):
        Trip(ok_small, [1, 2], 0, start_idx, end_idx)

    # This should be OK, starting and ending at the (only) depot.
    Trip(ok_small, [1, 2], 0, 0, 0)


@pytest.mark.parametrize("visits", [[], [1], [2, 3]])
def test_trip_length_and_visits(ok_small, visits: list[int]):
    """
    Tests that the trip length returns the number of client visits, and
    visits() the actual visits.
    """
    trip = Trip(ok_small, visits, 0, 0, 0)
    assert_equal(len(trip), len(visits))
    assert_equal(trip.visits(), visits)


@pytest.mark.parametrize("visits", [[1, 2, 3], [4, 3], [4], [1, 2, 3, 4]])
def test_single_route_and_trip_same_statistics(ok_small, visits: list[int]):
    """
    Tests that a single-trip route and just a trip agree on basic statistics
    about the route/trip.
    """
    trip = Trip(ok_small, visits, 0, 0, 0)
    route = Route(ok_small, visits, 0)

    # Structural attributes.
    assert_equal(len(trip), len(route))
    assert_equal(trip.vehicle_type(), route.vehicle_type())
    assert_equal(trip.start_depot(), route.start_depot())
    assert_equal(trip.end_depot(), route.end_depot())
    assert_equal(trip.visits(), route.visits())
    assert_allclose(trip.centroid(), route.centroid())

    # Distance- and cost-related statistics.
    assert_equal(trip.distance(), route.distance())
    assert_equal(trip.prizes(), route.prizes())

    # Duration-related statistics.
    assert_equal(trip.duration(), route.duration())
    assert_equal(trip.time_warp(), route.time_warp())
    assert_equal(trip.service_duration(), route.service_duration())
    assert_equal(trip.travel_duration(), route.travel_duration())
    assert_equal(trip.wait_duration(), route.wait_duration())
    assert_equal(trip.release_time(), route.release_time())

    # Load-related statistics.
    assert_equal(trip.delivery(), route.delivery())
    assert_equal(trip.pickup(), route.pickup())
    assert_equal(trip.excess_load(), route.excess_load())

    # Feasibility-related statistics.
    assert_equal(trip.is_feasible(), route.is_feasible())
    assert_equal(trip.has_excess_load(), route.has_excess_load())
    assert_equal(trip.has_time_warp(), route.has_time_warp())


def test_eq(ok_small_multi_depot):
    """
    Tests the trip's equality operator.
    """
    trip1 = Trip(ok_small_multi_depot, [2, 3], 0, 0, 0)
    trip2 = Trip(ok_small_multi_depot, [4], 0, 0, 0, previous=trip1)
    trip3 = Trip(ok_small_multi_depot, [2, 3], 0, 0, 0)

    assert_(trip1 == trip1)  # same object
    assert_(trip1 != trip2)  # different visits
    assert_(trip1 == trip3)  # same visits and start/end depots

    trip4 = Trip(ok_small_multi_depot, [2, 3], 1, 1, 1)
    assert_(trip1 != trip4)  # different vehicle type, start/end depots

    # And a few tests against things that are not trips, just to be sure that
    # there's also a type check in there somewhere.
    assert_(trip1 != 1)
    assert_(trip2 != "abc")
    assert_(trip3 != 5)


def test_previous(ok_small):
    """
    Tests that the trip correctly evaluates duration and time warp values that
    result from a previous trip.
    """
    veh_type = ok_small.vehicle_type(0).replace(reload_depots=[0])
    data = ok_small.replace(vehicle_types=[veh_type])

    prev = Trip(data, [2], 0, 0, 0)
    trip1 = Trip(data, [3], 0, 0, 0, previous=prev)
    trip2 = Trip(data, [3], 0, 0, 0)

    assert_equal(trip1.duration(), trip2.duration())
    assert_equal(trip1.service_duration(), trip2.service_duration())
    assert_equal(trip1.travel_duration(), trip2.travel_duration())

    # Trip1 starts after a trip visiting client 2. This implies we have a
    # full route of 0 -> 2 -> 0 -> 3 -> 0. Trip2, on the other hand, is just
    # 0 -> 3 -> 0. The data is as follows:
    # - Client 2: [12'000 early, 19'500 late], service 360
    # - Client 3: [8'400 early, 15'300 late], service 420
    # - Distance 2 -> 0: 1'965, 0 -> 2: 1'944, 0 -> 3: 1'931.
    # At the earliest, we arrive at client 2 at 12'000, leaving from the depot
    # at 10'056 (starting time of prev). Then we do service, and leave at
    # 12'360. We arrive at the depot at 14325, and immediately leave there
    # since depot service takes no time. This is the starting time for trip1.
    # Trip2, not having to account for a previous trip, can start at 8'400
    # minus the travel time to client 3, which amounts to 6'469.
    assert_equal(prev.start_time(), 10_056)
    assert_equal(trip1.start_time(), 14_325)
    assert_equal(trip2.start_time(), 6_469)

    # We arrive at client 3 at 16'256, which is after its time window closes at
    # 15'300. So there is time warp on trip1. Trip2, however, does not have
    # time warp.
    assert_equal(trip1.time_warp(), 956)
    assert_(not trip1.is_feasible())

    assert_equal(trip2.time_warp(), 0)
    assert_(trip2.is_feasible())

    # Because of the time warp there should be no slack on trip1, while there
    # is the full time window of client 2's worth of slack on trip2.
    assert_equal(trip1.slack(), 0)
    assert_equal(trip2.slack(), 6_900)

    # prev ends when trip1 starts, and trip1 and trip2 end after service, and
    # return to the depot. That's at 16'256 + 420 + 2'063 - 956 = 17'783 for
    # trip1, and 6'469 + 1'931 + 420 + 2'063 = 10'883 for trip2.
    assert_equal(prev.end_time(), 14_325)
    assert_equal(trip1.start_time(), prev.end_time())
    assert_equal(trip1.end_time(), 17_783)
    assert_equal(trip2.end_time(), 10_883)


def test_trip_iter_and_getitem(ok_small):
    """
    Tests that the trip is iterable and may be indexed.
    """
    trip = Trip(ok_small, [1, 2], 0, 0, 0)

    # Regular indexing gets the client visits.
    assert_equal(trip[0], 1)
    assert_equal(trip[1], 2)

    # Indexing from the end is also supported.
    assert_equal(trip[-1], 2)
    assert_equal(trip[-2], 1)

    # Negative indexing need to result in a proper index.
    with assert_raises(IndexError):
        trip[-3]

    # As should regular indexing.
    with assert_raises(IndexError):
        trip[2]

    # The trip is iterable, and iterating it returns the client visits.
    assert_equal(trip.visits(), [client for client in trip])

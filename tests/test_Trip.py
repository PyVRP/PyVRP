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


def test_raises_invalid_clients(ok_small):
    """
    Tests that the constructor raises when given a visits list that contains
    invalid clients.
    """
    assert_equal(ok_small.num_depots, 1)
    assert_equal(ok_small.num_clients, 4)

    with assert_raises(ValueError):
        Trip(ok_small, [0], 0)

    with assert_raises(ValueError):
        Trip(ok_small, [5], 0)


def test_trip_defaults_to_vehicle_start_end_depots(ok_small_multi_depot):
    """
    Tests that the constructor defaults to using the vehicle's start and end
    depots if no explicit trip start and end depots are passed.
    """
    trip1 = Trip(ok_small_multi_depot, [], 0)
    assert_equal(trip1.start_depot(), 0)
    assert_equal(trip1.end_depot(), 0)

    trip2 = Trip(ok_small_multi_depot, [], 1)
    assert_equal(trip2.start_depot(), 1)
    assert_equal(trip2.end_depot(), 1)


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
    about the route/trip, insofar those can be computed for just a trip in
    isolation.
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
    assert_equal(trip.service_duration(), route.service_duration())
    assert_equal(trip.travel_duration(), route.travel_duration())
    assert_equal(trip.release_time(), route.release_time())

    # Load-related statistics.
    assert_equal(trip.delivery(), route.delivery())
    assert_equal(trip.pickup(), route.pickup())
    assert_equal(trip.excess_load(), route.excess_load())
    assert_equal(trip.has_excess_load(), route.has_excess_load())


def test_eq(ok_small_multi_depot):
    """
    Tests the trip's equality operator.
    """
    trip1 = Trip(ok_small_multi_depot, [2, 3], 0, 0, 0)
    trip2 = Trip(ok_small_multi_depot, [4], 0, 0, 0)
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


def test_load(ok_small):
    """
    Tests load calculations for a small instance.
    """
    trip = Trip(ok_small, [1, 2, 3, 4], 0)
    assert_equal(trip.delivery(), [18])
    assert_equal(trip.pickup(), [0])
    assert_equal(trip.load(), [18])
    assert_equal(trip.excess_load(), [8])

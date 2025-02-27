import pytest
from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

from pyvrp import Route, Trip


@pytest.mark.parametrize(("start_idx", "end_idx"), [(1, 0), (0, 1)])
def test_trip_raises_for_invalid_depot_arguments(ok_small, start_idx, end_idx):
    """
    The constructor should raise when the depot arguments point to non-existing
    depots.
    """
    assert_equal(ok_small.num_depots, 1)
    with assert_raises(ValueError):
        Trip(ok_small, [1, 2], 0, start_idx, end_idx)

    # This should be OK, starting and ending at the (only) depot.
    Trip(ok_small, [1, 2], 0, 0, 0)


def test_raises_if_start_different_from_after_end(ok_small_multi_depot):
    """
    Tests that the trip constructor raises when we start from a different
    depot location than where the previous trip ended.
    """
    prev = Trip(ok_small_multi_depot, [2], 0, 1, 0)  # end at depot 0

    with assert_raises(ValueError):
        # A trip should start at the exact same place the previous trip ended.
        # If that's not the case (because the depots are not the same) then
        # the constructor should raise. Here, we start at depot 1, but prev
        # ended at depot 0.
        Trip(ok_small_multi_depot, [3], 0, 1, 0, after=prev)


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
    depot1 = 0
    depot2 = 1

    trip1 = Trip(ok_small_multi_depot, [2, 3], 0, depot1, depot1)
    trip2 = Trip(ok_small_multi_depot, [4], 0, depot1, depot2, after=trip1)
    trip3 = Trip(ok_small_multi_depot, [2, 3], 0, depot1, depot1)

    assert_(trip1 == trip1)  # same object
    assert_(trip1 != trip2)  # different visits, start/end locations
    assert_(trip1 == trip3)  # same visits and start/end locations

    # And a few tests against things that are not trips, just to be sure that
    # there's also a type check in there somewhere.
    assert_(trip1 != 1)
    assert_(trip2 != "abc")
    assert_(trip3 != 5)


def test_reload():
    """
    TODO
    """
    pass


def test_after():
    """
    TODO
    """
    pass

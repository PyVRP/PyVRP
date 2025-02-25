import pytest
from numpy.testing import assert_equal, assert_raises

from pyvrp import Depot, Reload, Route, Trip


@pytest.mark.parametrize(
    ("start", "end"),
    [
        (Depot(x=0, y=0), None),  # non-existing start depot
        (None, Depot(x=0, y=0)),  # non-existing end depot
        (None, Reload(depot=1)),  # there is no depot 1
        (Reload(depot=1), None),  # there is no depot 1
    ],
)
def test_trip_raises_for_invalid_start_end_arguments(ok_small, start, end):
    """
    The constructor should raise when given depot or reload arguments that
    point to non-existing depots.
    """
    if start is None:
        start = ok_small.location(0)

    if end is None:
        end = ok_small.location(0)

    with assert_raises(ValueError):
        Trip(ok_small, [1, 2], 0, start, end)

    # This should be OK, starting and ending at the depot.
    Trip(ok_small, [1, 2], 0, ok_small.location(0), ok_small.location(0))


def test_raises_if_start_is_wrong_or_missing_after_argument(ok_small):
    """
    Tests that the constructor raises when a trip starts at a reload location,
    without information about a previous trip that brought us there, or when
    the previous end and this start location are not the same.
    """
    reload1 = Reload(depot=0)
    prev = Trip(ok_small, [1], 0, ok_small.location(0), reload1)

    with assert_raises(ValueError):
        # Trip starts at a reload location, so this cannot be the first trip
        # in a route. But we didn't provide a previous trip, so something is
        # wrong: this trip cannot stand on its own.
        Trip(ok_small, [2], 0, reload1, ok_small.location(0))

    # Now that we provide the appropriate argument the constructor should work.
    Trip(ok_small, [2], 0, reload1, ok_small.location(0), after=prev)

    reload2 = Reload(depot=1)
    with assert_raises(ValueError):
        # A trip should start at the exact same place the previous trip ended.
        # If that's not the case (because the depots are not the same) then
        # the constructor should raise.
        Trip(ok_small, [2], 0, reload2, ok_small.location(0), after=prev)


@pytest.mark.parametrize("visits", [[], [1], [2, 3]])
def test_trip_length_and_visits(ok_small, visits: list[int]):
    """
    Tests that the trip length returns the number of client visits, and
    visits() the actual visits.
    """
    depot = ok_small.location(0)
    trip = Trip(ok_small, visits, 0, depot, depot)
    assert_equal(len(trip), len(visits))
    assert_equal(trip.visits(), visits)


@pytest.mark.parametrize("visits", [[1, 2, 3], [4, 3], [4], [1, 2, 3, 4]])
def test_single_route_and_trip_same_statistics(ok_small, visits: list[int]):
    """
    Tests that a single-trip route and just a trip agree on basic statistics
    about the route/trip.
    """
    depot = ok_small.location(0)
    trip = Trip(ok_small, visits, 0, depot, depot)
    route = Route(ok_small, visits, 0)

    # Structural attributes.
    assert_equal(len(trip), len(route))
    assert_equal(trip.vehicle_type(), route.vehicle_type())
    assert_equal(trip.start_depot(), route.start_depot())
    assert_equal(trip.end_depot(), route.end_depot())
    assert_equal(trip.visits(), route.visits())

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


# TODO

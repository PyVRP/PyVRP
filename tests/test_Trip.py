import pytest
from numpy.testing import assert_raises

from pyvrp import Depot, Reload, Trip


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


# TODO

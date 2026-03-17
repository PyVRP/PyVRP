import pickle

import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import Activity, ActivityType


def test_is_client_depot():
    """
    Tests the is_client() and is_depot() member functions.
    """
    client = Activity(ActivityType.CLIENT, 0)
    assert_(client.is_client())
    assert_(not client.is_depot())

    depot = Activity(ActivityType.DEPOT, 0)
    assert_(not depot.is_client())
    assert_(depot.is_depot())


def test_eq():
    """
    Tests the Activity's equality operator.
    """
    activity1 = Activity(ActivityType.DEPOT, 1)
    activity2 = Activity(ActivityType.CLIENT, 2)
    assert_(activity1 == activity1)
    assert_(activity2 != activity1)

    # Some non-Activity objects.
    assert_(activity1 != 1)
    assert_(activity1 != "test")


def test_pickle():
    """
    Tests pickling and unpickling an Activity object.
    """
    activity = Activity(ActivityType.DEPOT, 123)
    pickled = pickle.dumps(activity)
    unpickled = pickle.loads(pickled)
    assert_equal(activity, unpickled)


def test_description_init():
    """
    Tests the alternative, description-based constructor and string
    representation.
    """
    activity = Activity("D0")
    assert_(activity.is_depot())
    assert_equal(activity.idx, 0)
    assert_equal(str(activity), "D0")

    activity = Activity("C100")
    assert_(activity.is_client())
    assert_equal(activity.idx, 100)
    assert_equal(str(activity), "C100")


@pytest.mark.parametrize("description", ("", "D", "U0", "UA"))
def test_raises_unknown_description(description: str):
    """
    Tests that the description-based constructor raises for unknown or
    incomplete descriptions.
    """
    with assert_raises(ValueError):
        Activity(description)

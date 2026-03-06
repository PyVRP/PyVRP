import pickle

from numpy.testing import assert_, assert_equal

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


def test_unpacking():
    """
    Tests unpacking an activity into a (type, idx) tuple.
    """
    activity = Activity(ActivityType.CLIENT, 10)
    type, idx = activity
    assert_equal(type, ActivityType.CLIENT)
    assert_equal(idx, 10)


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

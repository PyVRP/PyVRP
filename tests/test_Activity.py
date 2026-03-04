import pickle

from numpy.testing import assert_, assert_equal

from pyvrp import Activity, ActivityType


def test_depot_activity():
    """
    Tests that a depot activity reports the correct type and index.
    """
    activity = Activity(ActivityType.DEPOT, 0)
    assert_equal(activity.type, ActivityType.DEPOT)
    assert_equal(activity.index, 0)
    assert_(activity.is_depot())
    assert_(not activity.is_client())


def test_client_activity():
    """
    Tests that a client activity reports the correct type and index.
    """
    activity = Activity(ActivityType.CLIENT, 5)
    assert_equal(activity.type, ActivityType.CLIENT)
    assert_equal(activity.index, 5)
    assert_(activity.is_client())
    assert_(not activity.is_depot())


def test_eq():
    """
    Tests the equality operator.
    """
    depot = Activity(ActivityType.DEPOT, 0)
    client = Activity(ActivityType.CLIENT, 1)

    assert_(depot == depot)
    assert_(client == client)
    assert_(depot != client)

    # Same type and index.
    assert_(Activity(ActivityType.CLIENT, 1) == client)

    # Same index, different type.
    assert_(Activity(ActivityType.DEPOT, 1) != client)

    # Same type, different index.
    assert_(Activity(ActivityType.CLIENT, 2) != client)

    # Against non-Activity objects.
    assert_(depot != 0)
    assert_(client != "abc")


def test_pickle():
    """
    Tests that Activity can be pickled and unpickled correctly.
    """
    activities = [
        Activity(ActivityType.DEPOT, 0),
        Activity(ActivityType.CLIENT, 7),
    ]
    for activity in activities:
        unpickled = pickle.loads(pickle.dumps(activity))
        assert_equal(activity, unpickled)

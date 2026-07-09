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


@pytest.mark.parametrize("description", ("", "D", "V0", "VA"))
def test_raises_unknown_description(description: str):
    """
    Tests that the description-based constructor raises for unknown or
    incomplete descriptions.
    """
    with assert_raises(ValueError):
        Activity(description)


def test_shipment():
    """
    Tests shipment activities, in particular pickup and delivery activities.
    """
    pickup = Activity("L0")
    assert_(pickup.is_shipment())
    assert_(pickup.is_pickup())
    assert_(not pickup.is_delivery())
    assert_equal(pickup.idx, 0)
    assert_equal(str(pickup), "L0")

    delivery = Activity("U0")
    assert_(delivery.is_shipment())
    assert_(not delivery.is_pickup())
    assert_(delivery.is_delivery())
    assert_equal(delivery.idx, 0)
    assert_equal(str(delivery), "U0")


def test_hash():
    """
    Tests that hashing activities results in different keys for different
    activities.
    """
    depot = Activity("D0")
    assert_equal(hash(depot), hash(depot))

    client1 = Activity("C0")
    client2 = Activity("C1")
    assert_(hash(client1) != hash(depot))
    assert_(hash(client1) != hash(client2))

import pickle

import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import Depot


@pytest.mark.parametrize(
    ("tw_early", "tw_late", "service_duration"),
    [
        (1, 0, 0),  # tw_early > tw_late
        (-1, 0, 0),  # tw_early < 0
        (0, -1, 0),  # tw_late < 0
        (0, 0, -1),  # service_duration < 0
    ],
)
def test_raises_for_invalid_data(
    tw_early: int,
    tw_late: int,
    service_duration: int,
):
    """
    Tests that an invalid depot configuration is not accepted.
    """
    with assert_raises(ValueError):
        Depot(0, tw_early, tw_late, service_duration)


def test_initialises_data_correctly():
    """
    Tests that the constructor correctly initialises its member data, and
    ensures the data is accessible from Python.
    """
    depot = Depot(
        location=2,
        tw_early=5,
        tw_late=7,
        service_duration=3,
        name="test",
    )

    assert_equal(depot.location, 2)
    assert_equal(depot.tw_early, 5)
    assert_equal(depot.tw_late, 7)
    assert_equal(depot.service_duration, 3)
    assert_equal(depot.name, "test")


def test_eq():
    """
    Tests the equality operator.
    """
    depot1 = Depot(location=0)
    depot2 = Depot(location=1)
    assert_(depot1 != depot2)

    # This depot is equivalent to depot1.
    depot3 = Depot(location=0)
    assert_(depot1 == depot3)
    assert_(depot3 == depot3)

    # And some things that are not depots.
    assert_(depot1 != "text")
    assert_(depot1 != 3)

    depot4 = Depot(location=0, name="test")
    assert_(depot1 != depot4)


def test_eq_name():
    """
    Tests that the equality operator considers names.
    """
    assert_(Depot(0, name="1") != Depot(0, name="2"))


def test_pickle():
    """
    Tests that depots can be serialised and unserialised.
    """
    before_pickle = Depot(location=0, name="test")
    bytes = pickle.dumps(before_pickle)
    assert_equal(pickle.loads(bytes), before_pickle)

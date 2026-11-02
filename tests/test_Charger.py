import pickle

from numpy.testing import assert_, assert_equal

from pyvrp import Charger


def test_attribute_access():
    """
    Tests that the charger's attributes are accessible.
    """
    charger = Charger(location=1, name="charger")
    assert_equal(charger.location, 1)
    assert_equal(charger.name, "charger")
    assert_equal(str(charger), "charger")


def test_default_values():
    """
    Tests that the name field is empty by default.
    """
    assert_equal(Charger(location=0).name, "")


def test_eq():
    """
    Tests the equality operator.
    """
    charger1 = Charger(location=0, name="test")
    charger2 = Charger(location=0, name="test")
    charger3 = Charger(location=1, name="test")
    charger4 = Charger(location=0, name="other")

    assert_(charger1 == charger2)
    assert_(charger1 != charger3)
    assert_(charger1 != charger4)

    # And some things that are not equal, because they're not chargers.
    assert_(charger1 != "test")
    assert_(charger1 != 0)


def test_pickle():
    """
    Tests that chargers can be serialised and unserialised.
    """
    before_pickle = Charger(location=1, name="test")
    bytes = pickle.dumps(before_pickle)
    assert_equal(pickle.loads(bytes), before_pickle)

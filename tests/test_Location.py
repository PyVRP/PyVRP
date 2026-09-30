from numpy.testing import assert_

from pyvrp import Location


def test_eq():
    """
    Tests the equality operator.
    """
    loc1 = Location(x=0, y=0, name="")
    loc2 = Location(x=0, y=1, name="")
    assert_(loc1 == loc1)
    assert_(loc2 != loc1)

    # Equivalent to loc1.
    loc3 = Location(x=0, y=0, name="")
    assert_(loc3 == loc1)

    # And some things that are not locations at all.
    assert_(loc1 != "test")
    assert_(loc1 != 1)


def test_eq_checks_names():
    """
    Tests that the name is also compared.
    """
    assert_(Location(0, 0, name="1") != Location(0, 0, name="2"))


def test_eq_accounts_for_small_measure_tolerances():
    """
    Internally, Location uses the Coordinate measure. That measure tolerances a
    little bit of floating-point error when comparing equality. This test
    checks that's OK.
    """
    original = Location(1.0, 0)
    rounded = Location(1.0 + 5e-10, 0)
    assert_(original == rounded)
    assert_(rounded == original)

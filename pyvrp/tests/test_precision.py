from numpy.testing import assert_

from pyvrp._precision import (
    equal_int,
    greater_equal_int,
    greater_int,
    less_equal_int,
    less_int,
)


def test_greater_float_tolerance():
    pass


def test_greater_int():
    assert_(greater_int(5, 4))
    assert_(not greater_int(4, 5))
    assert_(not greater_int(5, 5))
    assert_(greater_equal_int(5, 5))

    assert_(not greater_int(0, 0))
    assert_(greater_equal_int(0, 0))
    assert_(not greater_int(2**9, 2**9))
    assert_(not greater_int(-(2**9), -(2**9)))
    assert_(greater_int(-(2**9) + 1, -(2**9)))


def test_less_float_tolerance():
    pass


def test_less_int():
    assert_(less_int(4, 5))
    assert_(not less_int(5, 4))
    assert_(not less_int(5, 5))
    assert_(less_equal_int(5, 5))

    assert_(not less_int(0, 0))
    assert_(less_equal_int(0, 0))
    assert_(not less_int(2**9, 2**9))
    assert_(not less_int(-(2**9), -(2**9)))
    assert_(less_int(-(2**9) - 1, -(2**9)))


def test_equal_float_tolerance():
    pass


def test_equal_int():
    assert_(equal_int(0, 0))
    assert_(equal_int(1**9, 1**9))
    assert_(equal_int(-(1**9), -(1**9)))

    assert_(not equal_int(0, 1))
    assert_(not equal_int(1, 0))

    assert_(not equal_int(-(1**9), -(1**9) + 1))
    assert_(not equal_int(1**9, 1**9 - 1))

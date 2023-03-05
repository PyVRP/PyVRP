from math import inf

from numpy.testing import assert_

from pyvrp._precision import (
    equal_float,
    equal_int,
    greater_equal_float,
    greater_equal_int,
    greater_float,
    greater_int,
    less_equal_float,
    less_equal_int,
    less_float,
    less_int,
)


def test_greater_float_tolerance():
    assert_(greater_float(6.5, 6.4))
    assert_(greater_float(6.5, 6.49))
    assert_(greater_float(6.5, 6.499))
    assert_(greater_float(6.5, 6.4999))
    assert_(greater_float(6.5, 6.49999))

    # Difference here is smaller than one millionth of the two numbers (default
    # tolerance is one millionth).
    assert_(6.5 - 6.499999 <= 6.5 * 1e-6)
    assert_(not greater_float(6.5, 6.499999))
    assert_(greater_equal_float(6.5, 6.499999))

    # If we lower the tolerance, the check should pass.
    assert_(greater_float(6.5, 6.499999, tol=1e-9))

    # But not when the numbers are equal.
    assert_(not greater_float(6.5, 6.5, tol=1e-9))
    assert_(greater_equal_float(6.5, 6.5, tol=1e-9))
    assert_(not greater_float(0.0, 0.0, tol=1e-9))
    assert_(greater_equal_float(0.0, 0.0, tol=1e-9))
    assert_(not greater_float(213.41, 213.41, tol=1e-9))
    assert_(greater_equal_float(213.41, 213.41, tol=1e-9))
    assert_(not greater_float(inf, inf, tol=1e-9))


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
    assert_(less_float(6.4, 6.5))
    assert_(less_float(6.49, 6.5))
    assert_(less_float(6.499, 6.5))
    assert_(less_float(6.4999, 6.5))
    assert_(less_float(6.49999, 6.5))

    # Difference here is smaller than one millionth of the two numbers (default
    # tolerance is one millionth).
    assert_(6.5 - 6.499999 <= 6.5 * 1e-6)
    assert_(not less_float(6.499999, 6.5))
    assert_(less_equal_float(6.499999, 6.5))

    # If we lower the tolerance, the check should pass.
    assert_(less_float(6.499999, 6.5, tol=1e-9))

    # But not when the numbers are equal.
    assert_(not less_float(6.5, 6.5, tol=1e-9))
    assert_(less_equal_float(6.5, 6.5, tol=1e-9))
    assert_(not less_float(0.0, 0.0, tol=1e-9))
    assert_(less_equal_float(0.0, 0.0, tol=1e-9))
    assert_(not less_float(213.41, 213.41, tol=1e-9))
    assert_(less_equal_float(213.41, 213.41, tol=1e-9))
    assert_(not less_float(inf, inf, tol=1e-9))


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
    # These should all evaluate false since they're beyond the default
    # tolerance of one in a million.
    assert_(not equal_float(6.4, 6.5))
    assert_(not equal_float(6.49, 6.5))
    assert_(not equal_float(6.499, 6.5))
    assert_(not equal_float(6.4999, 6.5))
    assert_(not equal_float(6.49999, 6.5))

    assert_(not equal_float(6.5, 6.4))
    assert_(not equal_float(6.5, 6.49))
    assert_(not equal_float(6.5, 6.499))
    assert_(not equal_float(6.5, 6.4999))
    assert_(not equal_float(6.5, 6.49999))

    # But these are not, and should evaluate equal with the default tolerance.
    assert_(equal_float(6.499999, 6.5))
    assert_(equal_float(6.5, 6.499999))

    # But not any more when we lower the tolerance.
    assert_(not equal_float(6.499999, 6.5, tol=1e-9))
    assert_(not equal_float(6.5, 6.499999, tol=1e-9))


def test_equal_int():
    assert_(equal_int(0, 0))
    assert_(equal_int(1**9, 1**9))
    assert_(equal_int(-(1**9), -(1**9)))

    assert_(not equal_int(0, 1))
    assert_(not equal_int(1, 0))

    assert_(not equal_int(-(1**9), -(1**9) + 1))
    assert_(not equal_int(1**9, 1**9 - 1))

from numpy.testing import assert_

from pyvrp._precision import (
    equal_float,
    equal_int,
    greater_float,
    greater_int,
    smaller_float,
    smaller_int,
)


def test_equal_float_relative_tolerance():
    # These should all evaluate false since they're beyond the default relative
    # tolerance of 10^-6 (and the absolute tolerance of 10^-9).
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

    # But not any more when we lower the relative tolerance.
    assert_(not equal_float(6.499999, 6.5, rtol=1e-9))
    assert_(not equal_float(6.5, 6.499999, rtol=1e-9))


def test_equal_float_absolute_tolerance():
    # These should all evaluate false since they're beyond the default absolute
    # tolerance of 10^-9 (and the relative tolerance of 10^-6).
    assert_(not equal_float(0, 0.1))
    assert_(not equal_float(0, 0.01))
    assert_(not equal_float(0, 0.001))
    assert_(not equal_float(0, 0.0001))
    assert_(not equal_float(0, 0.00001))
    assert_(not equal_float(0, 0.000001))
    assert_(not equal_float(0, 0.0000001))

    assert_(not equal_float(0.1, 0))
    assert_(not equal_float(0.01, 0))
    assert_(not equal_float(0.001, 0))
    assert_(not equal_float(0.0001, 0))
    assert_(not equal_float(0.00001, 0))
    assert_(not equal_float(0.000001, 0))
    assert_(not equal_float(0.0000001, 0))

    assert_(not equal_float(0.65, 0.60))
    assert_(not equal_float(0.065, 0.060))
    assert_(not equal_float(0.0065, 0.0060))
    assert_(not equal_float(0.00065, 0.00060))
    assert_(not equal_float(0.000065, 0.000060))
    assert_(not equal_float(0.0000065, 0.0000060))
    assert_(not equal_float(0.00000065, 0.00000060))

    assert_(not equal_float(0.60, 0.65))
    assert_(not equal_float(0.060, 0.065))
    assert_(not equal_float(0.0060, 0.0065))
    assert_(not equal_float(0.00060, 0.00065))
    assert_(not equal_float(0.000060, 0.000065))
    assert_(not equal_float(0.0000060, 0.0000065))
    assert_(not equal_float(0.00000060, 0.00000065))

    # This would be tolerated
    assert_(equal_float(0, 0.00000001))
    assert_(equal_float(0, 0.000000001))
    assert_(equal_float(0.000000065, 0.000000060))

    assert_(equal_float(0.00000001, 0))
    assert_(equal_float(0.000000001, 0))
    assert_(equal_float(0.000000060, 0.000000065))

    # But not any more when we lower the absolute tolerance.
    assert_(not equal_float(0, 0.00000001, atol=1e-10))
    assert_(not equal_float(0, 0.000000001, atol=1e-10))
    assert_(not equal_float(0.000000065, 0.000000060, atol=1e-10))

    assert_(not equal_float(0.00000001, 0, atol=1e-10))
    assert_(not equal_float(0.000000001, 0, atol=1e-10))
    assert_(not equal_float(0.000000060, 0.000000065, atol=1e-10))


def test_equal_int():
    assert_(equal_int(0, 0))
    assert_(equal_int(1**9, 1**9))
    assert_(equal_int(-(1**9), -(1**9)))

    assert_(not equal_int(0, 1))
    assert_(not equal_int(1, 0))

    assert_(not equal_int(-(1**9), -(1**9) + 1))
    assert_(not equal_int(1**9, 1**9 - 1))


def test_smaller_float_relative_tolerance():
    # This tests exactly the same test cases as test_equal_float_relative_tol
    # All test cases expected to be equal cannot be (strictly) smaller/greater
    # all test cases expected not to be equal are either smaller or greater.

    # These should all evaluate false since they're beyond the default relative
    # tolerance of 10^-6 (and the absolute tolerance of 10^-9).
    assert_(smaller_float(6.4, 6.5))
    assert_(smaller_float(6.49, 6.5))
    assert_(smaller_float(6.499, 6.5))
    assert_(smaller_float(6.4999, 6.5))
    assert_(smaller_float(6.49999, 6.5))

    assert_(not smaller_float(6.5, 6.4))
    assert_(not smaller_float(6.5, 6.49))
    assert_(not smaller_float(6.5, 6.499))
    assert_(not smaller_float(6.5, 6.4999))
    assert_(not smaller_float(6.5, 6.49999))

    # But these are not, and should evaluate equal with the default tolerance
    # so are not considered smaller or greater.
    assert_(not smaller_float(6.499999, 6.5))
    assert_(not smaller_float(6.5, 6.499999))

    # If we lower the relative tolerance the numbers are no longer considered
    # equal so can be smaller or greater.
    assert_(smaller_float(6.499999, 6.5, rtol=1e-9))
    assert_(not smaller_float(6.5, 6.499999, rtol=1e-9))


def test_smaller_float_absolute_tolerance():
    # These should all not be considered equal since they're beyond the default
    # absolute tolerance of 10^-9 (and the relative tolerance of 10^-6) thus
    # may be smaller or greater depending on the sign of the difference.
    assert_(smaller_float(0, 0.1))
    assert_(smaller_float(0, 0.01))
    assert_(smaller_float(0, 0.001))
    assert_(smaller_float(0, 0.0001))
    assert_(smaller_float(0, 0.00001))
    assert_(smaller_float(0, 0.000001))
    assert_(smaller_float(0, 0.0000001))

    assert_(not smaller_float(0.1, 0))
    assert_(not smaller_float(0.01, 0))
    assert_(not smaller_float(0.001, 0))
    assert_(not smaller_float(0.0001, 0))
    assert_(not smaller_float(0.00001, 0))
    assert_(not smaller_float(0.000001, 0))
    assert_(not smaller_float(0.0000001, 0))

    assert_(not smaller_float(0.65, 0.60))
    assert_(not smaller_float(0.065, 0.060))
    assert_(not smaller_float(0.0065, 0.0060))
    assert_(not smaller_float(0.00065, 0.00060))
    assert_(not smaller_float(0.000065, 0.000060))
    assert_(not smaller_float(0.0000065, 0.0000060))
    assert_(not smaller_float(0.00000065, 0.00000060))

    assert_(smaller_float(0.60, 0.65))
    assert_(smaller_float(0.060, 0.065))
    assert_(smaller_float(0.0060, 0.0065))
    assert_(smaller_float(0.00060, 0.00065))
    assert_(smaller_float(0.000060, 0.000065))
    assert_(smaller_float(0.0000060, 0.0000065))
    assert_(smaller_float(0.00000060, 0.00000065))

    # But these are not, and should evaluate equal with the default tolerance
    # so are not considered smaller or greater.
    assert_(not smaller_float(0, 0.00000001))
    assert_(not smaller_float(0, 0.000000001))
    assert_(not smaller_float(0.000000065, 0.000000060))

    assert_(not smaller_float(0.00000001, 0))
    assert_(not smaller_float(0.000000001, 0))
    assert_(not smaller_float(0.000000060, 0.000000065))

    # If we lower the absolute tolerance the numbers are no longer considered
    # equal so can be smaller or greater.
    assert_(smaller_float(0, 0.00000001, atol=1e-10))
    assert_(smaller_float(0, 0.000000001, atol=1e-10))
    assert_(not smaller_float(0.000000065, 0.000000060, atol=1e-10))

    assert_(not smaller_float(0.00000001, 0, atol=1e-10))
    assert_(not smaller_float(0.000000001, 0, atol=1e-10))
    assert_(smaller_float(0.000000060, 0.000000065, atol=1e-10))


def test_smaller_int():
    assert_(not smaller_int(0, 0))
    assert_(not smaller_int(1**9, 1**9))
    assert_(not smaller_int(-(1**9), -(1**9)))

    assert_(smaller_int(0, 1))
    assert_(not smaller_int(1, 0))

    assert_(smaller_int(-(1**9), -(1**9) + 1))
    assert_(not smaller_int(-(1**9) + 1, -(1**9)))
    assert_(not smaller_int(1**9, 1**9 - 1))
    assert_(smaller_int(1**9 - 1, 1**9))


def test_greater_float_relative_tolerance():
    # This tests exactly the same test cases as test_equal_float_relative_tol
    # All test cases expected to be equal cannot be (strictly) smaller/greater
    # all test cases expected not to be equal are either smaller or greater.

    # These should all evaluate false since they're beyond the default relative
    # tolerance of 10^-6 (and the absolute tolerance of 10^-9).
    assert_(not greater_float(6.4, 6.5))
    assert_(not greater_float(6.49, 6.5))
    assert_(not greater_float(6.499, 6.5))
    assert_(not greater_float(6.4999, 6.5))
    assert_(not greater_float(6.49999, 6.5))

    assert_(greater_float(6.5, 6.4))
    assert_(greater_float(6.5, 6.49))
    assert_(greater_float(6.5, 6.499))
    assert_(greater_float(6.5, 6.4999))
    assert_(greater_float(6.5, 6.49999))

    # But these are not, and should evaluate equal with the default tolerance
    # so are not considered smaller or greater.
    assert_(not greater_float(6.499999, 6.5))
    assert_(not greater_float(6.5, 6.499999))

    # If we lower the relative tolerance the numbers are no longer considered
    # equal so can be smaller or greater.
    assert_(not greater_float(6.499999, 6.5, rtol=1e-9))
    assert_(greater_float(6.5, 6.499999, rtol=1e-9))


def test_greater_float_absolute_tolerance():
    # These should all not be considered equal since they're beyond the default
    # absolute tolerance of 10^-9 (and the relative tolerance of 10^-6) thus
    # may be smaller or greater depending on the sign of the difference.
    assert_(not greater_float(0, 0.1))
    assert_(not greater_float(0, 0.01))
    assert_(not greater_float(0, 0.001))
    assert_(not greater_float(0, 0.0001))
    assert_(not greater_float(0, 0.00001))
    assert_(not greater_float(0, 0.000001))
    assert_(not greater_float(0, 0.0000001))

    assert_(greater_float(0.1, 0))
    assert_(greater_float(0.01, 0))
    assert_(greater_float(0.001, 0))
    assert_(greater_float(0.0001, 0))
    assert_(greater_float(0.00001, 0))
    assert_(greater_float(0.000001, 0))
    assert_(greater_float(0.0000001, 0))

    assert_(greater_float(0.65, 0.60))
    assert_(greater_float(0.065, 0.060))
    assert_(greater_float(0.0065, 0.0060))
    assert_(greater_float(0.00065, 0.00060))
    assert_(greater_float(0.000065, 0.000060))
    assert_(greater_float(0.0000065, 0.0000060))
    assert_(greater_float(0.00000065, 0.00000060))

    assert_(not greater_float(0.60, 0.65))
    assert_(not greater_float(0.060, 0.065))
    assert_(not greater_float(0.0060, 0.0065))
    assert_(not greater_float(0.00060, 0.00065))
    assert_(not greater_float(0.000060, 0.000065))
    assert_(not greater_float(0.0000060, 0.0000065))
    assert_(not greater_float(0.00000060, 0.00000065))

    # But these are not, and should evaluate equal with the default tolerance
    # so are not considered smaller or greater.
    assert_(not greater_float(0, 0.00000001))
    assert_(not greater_float(0, 0.000000001))
    assert_(not greater_float(0.000000065, 0.000000060))

    assert_(not greater_float(0.00000001, 0))
    assert_(not greater_float(0.000000001, 0))
    assert_(not greater_float(0.000000060, 0.000000065))

    # If we lower the absolute tolerance the numbers are no longer considered
    # equal so can be smaller or greater.
    assert_(not greater_float(0, 0.00000001, atol=1e-10))
    assert_(not greater_float(0, 0.000000001, atol=1e-10))
    assert_(greater_float(0.000000065, 0.000000060, atol=1e-10))

    assert_(greater_float(0.00000001, 0, atol=1e-10))
    assert_(greater_float(0.000000001, 0, atol=1e-10))
    assert_(not greater_float(0.000000060, 0.000000065, atol=1e-10))


def test_greater_int():
    assert_(not greater_int(0, 0))
    assert_(not greater_int(1**9, 1**9))
    assert_(not greater_int(-(1**9), -(1**9)))

    assert_(not greater_int(0, 1))
    assert_(greater_int(1, 0))

    assert_(not greater_int(-(1**9), -(1**9) + 1))
    assert_(greater_int(-(1**9) + 1, -(1**9)))
    assert_(greater_int(1**9, 1**9 - 1))
    assert_(not greater_int(1**9 - 1, 1**9))

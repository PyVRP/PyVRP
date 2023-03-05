from numpy.testing import assert_

from pyvrp._precision import equal_float, equal_int


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

import ctypes

from numpy.testing import assert_equal

from pyvrp._lib.hgspy import XorShift128


def test_bounds():
    assert_equal(XorShift128.min(), 0)

    bits = 8 * ctypes.sizeof(ctypes.c_uint)
    assert_equal(XorShift128.max(), 2**bits - 1)


def test_call():
    rng = XorShift128(seed=42)

    assert_equal(rng(), 2386648076)
    assert_equal(rng(), 1236469084)

    rng = XorShift128(seed=43)

    assert_equal(rng(), 2386648077)
    assert_equal(rng(), 1236469085)


def test_randint():
    rng = XorShift128(seed=42)

    assert_equal(rng.randint(100), 2386648076 % 100)
    assert_equal(rng.randint(100), 1236469084 % 100)

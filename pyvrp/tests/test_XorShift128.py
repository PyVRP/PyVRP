import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal
from pytest import mark

from pyvrp import XorShift128


def test_bounds():
    assert_equal(XorShift128.min(), 0)
    assert_equal(XorShift128.max(), np.iinfo(np.uint32).max)


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


@mark.parametrize("seed", [2, 10, 42])
def test_rand(seed: int):
    rng = XorShift128(seed)
    sample = np.array([rng.rand() for _ in range(10_000)])

    # The sample should be almost uniform, so mean 1/ 2 and variance 1 / 12,
    # and every realisation needs to be in [0, 1].
    assert_allclose(sample.mean(), 1 / 2, atol=1e-3)
    assert_allclose(sample.var(), 1 / 12, atol=1e-3)
    assert_(0 <= sample.min() < sample.max() <= 1)

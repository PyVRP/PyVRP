from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.stop import MaxRuntime
from tests.helpers import sleep


def test_attribute_max_runtime():
    """
    Tests that the ``max_runtime`` attribute is set correctly.
    """
    max_runtime = 100
    stop = MaxRuntime(max_runtime)

    assert_equal(stop.max_runtime, max_runtime)


@mark.parametrize("max_runtime", [-0.001, -1, -10.1])
def test_raise_negative_parameters(max_runtime: float):
    """
    Maximum runtime may not be negative.
    """
    with assert_raises(ValueError):
        MaxRuntime(max_runtime)


@mark.parametrize("max_runtime", [0.001, 1, 10.1])
def test_valid_parameters(max_runtime: float):
    """
    Does not raise for non-negative parameters.
    """
    MaxRuntime(max_runtime)


@mark.parametrize("max_runtime", [0.01, 0.05, 0.10])
def test_before_max_runtime(max_runtime):
    """
    Tests that ``MaxRuntime`` does not stop *before* the given runtime has
    passed.
    """
    stop = MaxRuntime(max_runtime)

    for _ in range(100):
        assert_(not stop(1))


@mark.parametrize("max_runtime", [0.01, 0.05, 0.10])
def test_after_max_runtime(max_runtime):
    """
    Tests that after the max runtime has passed, ``MaxRuntime`` stops.
    """
    stop = MaxRuntime(max_runtime)
    assert_(not stop(1))  # trigger the first time measurement

    sleep(max_runtime)

    for _ in range(100):
        assert_(stop(1))

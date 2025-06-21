from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.stop import MaxRuntime
from tests.helpers import sleep


def test_attribute_max_runtime():
    """
    Tests that the ``max_runtime`` attribute is set correctly.
    """
    stop = MaxRuntime(1.23)
    assert_equal(stop.max_runtime, 1.23)


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


def test_fraction_remaining():
    """
    Tests that calling ``fraction_remaining()`` returns the correct values.
    """
    stop = MaxRuntime(1)
    assert_equal(stop.fraction_remaining(), 1)

    stop(0)
    assert_(0 < stop.fraction_remaining() < 1)


def test_fraction_remaining_edge_cases():
    """
    Tests that ``fraction_remaining()`` works for edge cases.
    """
    # Zero max runtime should result in zero fraction remaining from the start.
    stop = MaxRuntime(0)
    assert_equal(stop.fraction_remaining(), 0)

    # Fraction remaining should not go below zero.
    stop(0)
    assert_equal(stop.fraction_remaining(), 0)

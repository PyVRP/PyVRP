from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.stop import MaxIterations


def test_attribute_max_iterations():
    """
    Tests that the ``max_iterations`` attribute is set correctly.
    """
    stop = MaxIterations(100)
    assert_equal(stop.max_iterations, 100)


@mark.parametrize("max_iterations", [-1, -42, -10000])
def test_raise_negative_parameters(max_iterations: int):
    """
    Maximum iterations cannot be negative.
    """
    with assert_raises(ValueError):
        MaxIterations(max_iterations)


@mark.parametrize("max_iterations", [1, 42, 10000])
def test_does_not_raise(max_iterations: int):
    """
    Valid parameters should not raise.
    """
    MaxIterations(max_iterations)


def test_before_max_iterations():
    """
    Tests that ``MaxIterations`` does not stop *before* the number of
    iterations have passed.
    """
    stop = MaxIterations(100)

    for _ in range(100):  # stop *after* 100 iterations, not before
        assert_(not stop(1))


def test_after_max_iterations():
    """
    Tests that after the number of iterations have passed, ``MaxIterations``
    stops.
    """
    stop = MaxIterations(100)

    for _ in range(100):
        assert_(not stop(1))

    for _ in range(100):
        assert_(stop(1))


def test_fraction_remaining():
    """
    Tests that calling ``fraction_remaining()`` returns the correct values.
    """
    stop = MaxIterations(100)
    assert_equal(stop.fraction_remaining(), 1)

    stop(0)
    assert_equal(stop.fraction_remaining(), 0.99)


def test_fraction_remaining_edge_cases():
    """
    Tests that ``fraction_remaining()`` works for edge cases.
    """
    # Zero max iterations should result in zero fraction remaining from the
    # start.
    stop = MaxIterations(0)
    assert_equal(stop.fraction_remaining(), 0)

    # Fraction remaining should also not go below zero.
    stop(0)
    assert_equal(stop.fraction_remaining(), 0)

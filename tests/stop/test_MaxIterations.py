from numpy.testing import assert_, assert_raises
from pytest import mark

from pyvrp.stop import MaxIterations


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

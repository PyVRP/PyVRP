from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.stop import (
    FirstFeasible,
    MaxIterations,
    MaxRuntime,
    MultipleCriteria,
)
from tests.helpers import sleep


def test_raises_if_empty():
    """
    Tests that passing no criteria results in an error.
    """
    with assert_raises(ValueError):
        MultipleCriteria([])


def test_stops_if_zero_max_iterations():
    """
    Tests that the stopping criterion stops immediately if zero maximum
    iterations are allowed.
    """
    stop = MultipleCriteria([MaxIterations(0), MaxRuntime(0.100)])
    assert_(stop(1))


@mark.parametrize("max_runtime", [0.01, 0.05, 0.10])
def test_before_max_runtime(max_runtime):
    """
    Tests that the stopping criterion allows iterations if the maximum runtime
    has not yet been violated.
    """
    stop = MultipleCriteria([MaxIterations(101), MaxRuntime(max_runtime)])

    for _ in range(100):
        assert_(not stop(1))


@mark.parametrize("max_runtime", [0.01, 0.05, 0.10])
def test_after_max_runtime(max_runtime):
    """
    Tests that stopping criterion stops after maximum runtime, regardless
    of maximum number of iterations.
    """
    stop = MultipleCriteria([MaxIterations(101), MaxRuntime(max_runtime)])
    assert_(not stop(1))  # trigger the first time measurement

    sleep(max_runtime)

    for _ in range(100):
        assert_(stop(1))


def test_fraction_remaining():
    """
    Tests that calling ``MultipleCriteria.fraction_remaining()`` returns the
    minimum value among its criteria.
    """
    # MaxIterations is the most restrictive stopping criterion.
    stop = MultipleCriteria(
        [MaxIterations(1), MaxRuntime(10), FirstFeasible()]
    )
    assert_equal(stop.fraction_remaining(), 1)

    stop(0)
    assert_equal(stop.fraction_remaining(), 0)

    # Return None when no criteria have a meaningful fraction remaining.
    stop = MultipleCriteria([FirstFeasible()])
    assert_equal(stop.fraction_remaining(), None)

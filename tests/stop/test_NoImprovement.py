from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.stop import NoImprovement


@mark.parametrize("max_iterations", [-10, -100, -1000])
def test_raise_negative_parameters(max_iterations: int):
    """
    max_iterations cannot be negative.
    """
    with assert_raises(ValueError):
        NoImprovement(max_iterations)


def test_zero_max_iterations():
    """
    Test if setting max_iterations to zero always stops.
    """
    stop = NoImprovement(0)

    assert_(stop(1))
    assert_(stop(1))


def test_one_max_iterations():
    """
    Test if setting max_iterations to one only stops when a non-improving
    solution has been found.
    """
    stop = NoImprovement(1)

    assert_(not stop(2))
    assert_(not stop(1))
    assert_(stop(1))


@mark.parametrize("n", [10, 100, 1000])
def test_n_max_iterations_non_improving(n):
    """
    Test if setting max_iterations to n correctly stops with non-improving
    solutions. The first n iterations should not stop, but after that, the
    criterion should stop.
    """
    stop = NoImprovement(n)

    for _ in range(n):
        assert_(not stop(1))

    for _ in range(n):
        assert_(stop(1))


@mark.parametrize(("n", "k"), [(10, 2), (100, 20), (1000, 200)])
def test_n_max_iterations_with_single_improvement(n, k):
    """
    Test if setting max_iterations to n correctly stops with a sequence of
    solutions, when the k-th solution is improving and the other solutions
    are non-improving. The first n + k - 1 iterations should not stop. After
    that, the criterion should stop.
    """
    stop = NoImprovement(n)

    for _ in range(k):
        assert_(not stop(2))

    for _ in range(n):
        assert_(not stop(1))

    for _ in range(n):
        assert_(stop(1))


def test_fraction_remaining():
    """
    Tests that calling ``fraction_remaining()`` returns the correct values.
    """
    stop = NoImprovement(100)
    assert_equal(stop.fraction_remaining(), 1)

    # First call registers initial best solution and resets internal counter.
    stop(1)
    assert_equal(stop.fraction_remaining(), 1)

    # New solution is not improving, so fraction remaining should decrease.
    stop(2)
    assert_equal(stop.fraction_remaining(), 0.99)

    # Found a new best solution, so the iteration counter resets and so does
    # the fraction remaining.
    stop(0)
    assert_equal(stop.fraction_remaining(), 1)


def test_fraction_remaining_zero_budget():
    """
    Tests that ``fraction_remaining()`` works correctly when max iterations is
    set to zero.
    """
    # Zero max iterations should result in zero fraction remaining from the
    # start.
    stop = NoImprovement(0)
    assert_equal(stop.fraction_remaining(), 0)

    # Fraction remaining should also not go below zero.
    stop(0)
    assert_equal(stop.fraction_remaining(), 0)


def test_fraction_remaining_infinite_budget():
    """
    Tests that ``fraction_remaining()`` works correctly when max iterations is
    set to infinity, which is used in PyVRP's CLI.
    """
    stop = NoImprovement(float("inf"))
    assert_equal(stop.fraction_remaining(), 1)

    stop(0)
    assert_equal(stop.fraction_remaining(), 1)

    stop(0)
    assert_equal(stop.fraction_remaining(), 1)

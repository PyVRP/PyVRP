from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import PiecewiseLinearFunction


def test_call():
    """
    Tests calling the piecewise linear function on various segments.
    """
    fn = PiecewiseLinearFunction([5, 10, 20], [(1, 2), (11, 3), (26, 4)])

    # 1st segment, defined for values < 5.
    assert_equal(fn(-100), 1 + 2 * -100)
    assert_equal(fn(0), 1 + 2 * 0)
    assert_equal(fn(4), 1 + 2 * 4)

    # 2nd segment, defined for values in [5, 10).
    assert_equal(fn(5), 11 + 3 * 5)
    assert_equal(fn(9), 11 + 3 * 9)

    # 3rd segment, defined for values >= 10.
    assert_equal(fn(10), 26 + 4 * 10)
    assert_equal(fn(13), 26 + 4 * 13)
    assert_equal(fn(100), 26 + 4 * 100)


def test_zero():
    """
    Tests the piecewise linear function with zero slope and/or intercept.
    """
    fn = PiecewiseLinearFunction([0], [(0, 7)])  # zero intercept
    assert_equal(fn(-1), -7)
    assert_equal(fn(0), 0)
    assert_equal(fn(1), 7)

    fn = PiecewiseLinearFunction([0], [(7, 0)])  # zero slope (constant)
    assert_equal(fn(-5), 7)
    assert_equal(fn(0), 7)
    assert_equal(fn(5), 7)

    fn = PiecewiseLinearFunction([0], [(0, 0)])  # all zero
    assert_equal(fn(-100), 0)
    assert_equal(fn(0), 0)
    assert_equal(fn(100), 0)


def test_piecewise_linear_function_raises_inconsisten_argument_sizes():
    """
    Tests that the piecewise linear function cannot be constructed with
    inconsistently sized arguments.
    """
    with assert_raises(ValueError):  # need at least one segment
        PiecewiseLinearFunction([], [])

    with assert_raises(ValueError):  # argument sizes must match
        PiecewiseLinearFunction([0], [])

    with assert_raises(ValueError):  # argument sizes must match
        PiecewiseLinearFunction([], [(0, 0)])

    with assert_raises(ValueError):  # also when both arguments are not empty
        PiecewiseLinearFunction([0, 1, 2], [(0, 0), (0, 0)])


def test_breakpoints_and_segments_properties():
    """
    Tests getting the breakpoints and segments.
    """
    fn = PiecewiseLinearFunction([0, 5], [(1, 2), (11, 3)])
    assert_equal(fn.breakpoints, [0, 5])
    assert_equal(fn.segments, [(1, 2), (11, 3)])


def test_eq():
    """
    Tests equality comparisons between various functions.
    """
    fn1 = PiecewiseLinearFunction([5], [(1, 2)])
    fn2 = PiecewiseLinearFunction([5], [(1, 2)])
    fn3 = PiecewiseLinearFunction([5], [(1, 3)])
    fn4 = PiecewiseLinearFunction([6], [(1, 2)])

    assert_(fn1 == fn2)
    assert_(fn1 != fn3)
    assert_(fn1 != fn4)
    assert_(fn1 != "string")


def test_raises_unsorted_breakpoints():
    """
    Tests that the piecewise linear function cannot be constructed with
    unsorted breakpoints, or breakpoints that are not strictly increasing.
    """
    with assert_raises(ValueError):  # unsorted
        PiecewiseLinearFunction([2, 1, 3], [(1, 1), (2, 2), (3, 3)])

    with assert_raises(ValueError):  # not strictly increasing
        PiecewiseLinearFunction([1, 1], [(1, 1), (2, 2)])

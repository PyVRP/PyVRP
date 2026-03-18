import pickle

import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import PiecewiseLinearFunction


def test_call():
    """
    Tests calling the piecewise linear function on various segments.
    """
    fn = PiecewiseLinearFunction([5, 10], [(1, 2), (11, 3), (26, 4)])

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
    fn = PiecewiseLinearFunction([], [(0, 7)])  # zero intercept
    assert_equal(fn(-1), -7)
    assert_equal(fn(0), 0)
    assert_equal(fn(1), 7)

    fn = PiecewiseLinearFunction([], [(7, 0)])  # zero slope (constant)
    assert_equal(fn(-5), 7)
    assert_equal(fn(0), 7)
    assert_equal(fn(5), 7)

    fn = PiecewiseLinearFunction([], [(0, 0)])  # all zero
    assert_equal(fn(-100), 0)
    assert_equal(fn(0), 0)
    assert_equal(fn(100), 0)


def test_piecewise_linear_function_raises_inconsistent_argument_sizes():
    """
    Tests that the piecewise linear function cannot be constructed with
    inconsistently sized arguments.
    """
    with assert_raises(ValueError):  # need at least one segment
        PiecewiseLinearFunction([], [])

    with assert_raises(ValueError):  # need one more segment than breakpoints
        PiecewiseLinearFunction([0], [])

    with assert_raises(ValueError):  # also when both arguments are not empty
        PiecewiseLinearFunction([1, 2], [(0, 0), (0, 0)])


def test_breakpoints_and_segments_properties():
    """
    Tests getting the breakpoints and segments.
    """
    fn = PiecewiseLinearFunction([5], [(1, 2), (11, 3)])
    assert_equal(fn.breakpoints, [5])
    assert_equal(fn.segments, [(1, 2), (11, 3)])


def test_eq():
    """
    Tests equality comparisons between various functions.
    """
    fn1 = PiecewiseLinearFunction([5], [(1, 2), (2, 4)])
    fn2 = PiecewiseLinearFunction([5], [(1, 2), (2, 4)])
    fn3 = PiecewiseLinearFunction([5], [(1, 3), (2, 4)])
    fn4 = PiecewiseLinearFunction([6], [(1, 2), (2, 4)])

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
        PiecewiseLinearFunction([2, 1, 3], [(1, 1), (2, 2), (3, 3), (4, 4)])

    with assert_raises(ValueError):  # not strictly increasing
        PiecewiseLinearFunction([1, 1], [(1, 1), (2, 2), (2, 2)])


@pytest.mark.parametrize(
    ("points", "exp_breakpoints", "exp_segments"),
    [
        ([(0, 0), (1, 0)], [], [(0, 0)]),
        ([(0, 0), (1, 1)], [], [(0, 1)]),
        ([(0, 0), (2, 4), (2, 6), (10, 14)], [2], [(0, 2), (4, 1)]),
    ],
)
def test_points_constructor(
    points: list[tuple[int, int]],
    exp_breakpoints: list[int],
    exp_segments: list[tuple[int, int]],
):
    """
    Tests constructing a piecewise linear function from points, rather than
    from breakpoints and segments. The points should map to the correct
    breakpoints and segments.
    """
    points_fn = PiecewiseLinearFunction(points=points)
    segment_fn = PiecewiseLinearFunction(exp_breakpoints, exp_segments)
    assert_equal(points_fn, segment_fn)


def test_raises_point_constructor():
    """
    Tests that the points constructor raises for invalid input.
    """
    with assert_raises(ValueError):  # need at least two points
        PiecewiseLinearFunction(points=[(0, 0)])

    with assert_raises(ValueError):  # first two points must have different x
        PiecewiseLinearFunction(points=[(0, 0), (0, 0), (10, 10)])

    with assert_raises(ValueError):  # last two points must have different x
        PiecewiseLinearFunction(points=[(1, 0), (2, 0), (2, 0)])

    with assert_raises(ValueError):  # points must be non-decreasing in x
        PiecewiseLinearFunction(points=[(0, 0), (-1, 0)])

    with assert_raises(ValueError):  # slope must be integral - 3/2 is not
        PiecewiseLinearFunction(points=[(0, 0), (2, 3)])


@pytest.mark.parametrize(
    ("points", "expected"),
    [
        ([(0, 0), (10, 10), (10, 13), (11, 15)], True),  # non-decreasing
        ([(0, 0), (1, -1)], False),  # negative slope
        ([(0, 0), (5, 10), (5, 7), (6, 9)], False),  # jump down at x = 5
        ([(0, 0), (1, 0)], True),  # constant function
    ],
)
def test_is_monotonically_increasing(
    points: list[tuple[int, int]], expected: bool
):
    """
    Tests is_monotonically_increasing for a few cases, including with jumps.
    """
    fn = PiecewiseLinearFunction(points=points)
    assert_equal(fn.is_monotonically_increasing(), expected)


def test_pickle():
    """
    Tests that piecewise linear functions can be pickled and unpickled
    correctly.
    """
    fn = PiecewiseLinearFunction(points=[(0, 0), (5, 5), (5, 8), (6, 18)])

    pickled = pickle.dumps(fn)
    unpickled = pickle.loads(pickled)
    assert_equal(fn, unpickled)

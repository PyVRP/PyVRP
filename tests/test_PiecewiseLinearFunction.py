from numpy.testing import assert_equal
from pytest import raises as assert_raises

from pyvrp import PiecewiseLinearFunction


def test_piecewise_linear_function_evaluates_segments():
    fn = PiecewiseLinearFunction([0, 5, 10, 20], [(1, 2), (11, 3), (26, 4)])

    assert_equal(fn(0), 1)
    assert_equal(fn(4), 9)
    assert_equal(fn(5), 11)
    assert_equal(fn(9), 23)
    assert_equal(fn(10), 26)
    assert_equal(fn(13), 38)
    assert_equal(fn(20), 66)


def test_piecewise_linear_function_raises_invalid_data():
    with assert_raises(ValueError):
        PiecewiseLinearFunction([], [])

    with assert_raises(ValueError):
        PiecewiseLinearFunction([0], [])

    with assert_raises(ValueError):
        PiecewiseLinearFunction([0, 1], [])

    with assert_raises(ValueError):
        PiecewiseLinearFunction([0, 1, 1], [(0, 0), (0, 0)])


def test_piecewise_linear_function_raises_for_out_of_domain_query():
    fn = PiecewiseLinearFunction([5, 10], [(1, 2)])

    with assert_raises(ValueError, match="x must be within function domain."):
        fn(4)

    with assert_raises(ValueError, match="x must be within function domain."):
        fn(11)

    assert_equal(fn(10), 11)


def test_piecewise_linear_function_exposes_breakpoints_and_segments():
    fn = PiecewiseLinearFunction([0, 5, 10], [(1, 2), (11, 3)])

    assert_equal(fn.breakpoints, [0, 5, 10])
    assert_equal(fn.segments, [(1, 2), (11, 3)])


def test_piecewise_linear_function_equality():
    fn1 = PiecewiseLinearFunction([0, 5], [(1, 2)])
    fn2 = PiecewiseLinearFunction([0, 5], [(1, 2)])
    fn3 = PiecewiseLinearFunction([0, 5], [(1, 3)])
    fn4 = PiecewiseLinearFunction([0, 6], [(1, 2)])
    fn5 = PiecewiseLinearFunction([0, 5], [(2, 2)])

    assert fn1 == fn2
    assert fn1 != fn3
    assert fn1 != fn4
    assert fn1 != fn5


def test_piecewise_linear_function_raises_unsorted_breakpoints():
    with assert_raises(
        ValueError,
        match="breakpoints must be strictly increasing.",
    ):
        PiecewiseLinearFunction([0, 2, 1], [(1, 1), (2, 2)])

import numpy as np
from numpy.testing import assert_equal
from pytest import raises as assert_raises

from pyvrp import (
    DurationCostFunction,
    PiecewiseLinearFunction,
)

MAX_INT64 = int(np.iinfo(np.int64).max)
MIN_INT64 = int(np.iinfo(np.int64).min)


def test_piecewise_linear_function_evaluates_segments():
    fn = PiecewiseLinearFunction([0, 5, 10], [2, 3, 4], intercept=1)

    assert_equal(fn(0), 1)  # 0 * 2 + 1
    assert_equal(fn(4), 9)  # 4 * 2 + 1
    assert_equal(fn(5), 11)  # 5 * 2 + 1
    assert_equal(fn(9), 23)  # 5 * 2 + 4 * 3 + 1
    assert_equal(fn(10), 26)  # 5 * 2 + 5 * 3 + 1
    assert_equal(fn(13), 38)  # 5 * 2 + 5 * 3 + 3 * 4 + 1


def test_piecewise_linear_function_raises_invalid_data():
    with assert_raises(ValueError):
        # At least one breakpoint and slope required.
        PiecewiseLinearFunction([], [])

    with assert_raises(ValueError):
        # Different number of breakpoints and slopes.
        PiecewiseLinearFunction([0, 1], [0])

    with assert_raises(ValueError):
        # Duplicate breakpoints.
        PiecewiseLinearFunction([0, 1, 1], [0, 0, 0])


def test_piecewise_linear_function_raises_for_left_of_domain_query():
    fn = PiecewiseLinearFunction([5, 10], [2, 3], intercept=1)

    with assert_raises(ValueError, match="x must be >= first breakpoint."):
        fn(4)


def test_piecewise_linear_function_exposes_breakpoints_slopes_values():
    fn = PiecewiseLinearFunction([0, 5, 10], [2, 3, 4], intercept=1)

    assert_equal(fn.breakpoints, [0, 5, 10])
    assert_equal(fn.slopes, [2, 3, 4])
    assert_equal(fn.values, [1, 11, 26])
    assert_equal(fn.intercept, 1)


def test_piecewise_linear_function_is_zero():
    assert PiecewiseLinearFunction([0], [0], intercept=0).is_zero()
    assert not PiecewiseLinearFunction([0], [1], intercept=0).is_zero()
    assert not PiecewiseLinearFunction([0], [0], intercept=1).is_zero()


def test_piecewise_linear_function_equality():
    fn1 = PiecewiseLinearFunction([0, 5], [2, 3], intercept=1)
    fn2 = PiecewiseLinearFunction([0, 5], [2, 3], intercept=1)
    fn3 = PiecewiseLinearFunction([0, 5], [2, 4], intercept=1)
    fn4 = PiecewiseLinearFunction([0, 6], [2, 3], intercept=1)
    fn5 = PiecewiseLinearFunction([0, 5], [2, 3], intercept=2)

    assert fn1 == fn2
    assert fn1 != fn3
    assert fn1 != fn4
    assert fn1 != fn5


def test_piecewise_linear_function_raises_unsorted_breakpoints():
    with assert_raises(ValueError, match="breakpoints must be sorted."):
        PiecewiseLinearFunction([0, 2, 1], [1, 1, 1])


def test_piecewise_linear_function_raises_mul_overflow_in_constructor():
    with assert_raises(
        OverflowError,
        match="PiecewiseLinearFunction multiplication overflow.",
    ):
        PiecewiseLinearFunction([0, 2], [MAX_INT64, 0], intercept=0)


def test_piecewise_linear_function_raises_add_overflow_in_constructor():
    with assert_raises(
        OverflowError,
        match="PiecewiseLinearFunction addition overflow.",
    ):
        PiecewiseLinearFunction([0, 1], [1, 0], intercept=MAX_INT64)


def test_piecewise_linear_function_raises_neg_add_overflow_in_constructor():
    with assert_raises(
        OverflowError,
        match="PiecewiseLinearFunction addition overflow.",
    ):
        PiecewiseLinearFunction([0, 1], [-1, 0], intercept=MIN_INT64)


def test_piecewise_linear_function_raises_mul_overflow_on_call():
    fn = PiecewiseLinearFunction([0, 1], [1, MAX_INT64 // 2 + 1], intercept=0)

    with assert_raises(
        OverflowError,
        match="PiecewiseLinearFunction multiplication overflow.",
    ):
        fn(3)


def test_piecewise_linear_function_raises_neg_mul_overflow_on_call():
    fn = PiecewiseLinearFunction([0, 1], [0, MIN_INT64 // 2 - 1], intercept=0)

    with assert_raises(
        OverflowError,
        match="PiecewiseLinearFunction multiplication overflow.",
    ):
        fn(3)


def test_piecewise_linear_function_raises_add_overflow_on_call():
    fn = PiecewiseLinearFunction([0, 1], [MAX_INT64 - 1, 2], intercept=0)

    with assert_raises(
        OverflowError,
        match="PiecewiseLinearFunction addition overflow.",
    ):
        fn(2)


def test_piecewise_linear_function_raises_neg_add_overflow_on_call():
    fn = PiecewiseLinearFunction([0, 1], [MIN_INT64 + 1, -2], intercept=0)

    with assert_raises(
        OverflowError,
        match="PiecewiseLinearFunction addition overflow.",
    ):
        fn(2)


def test_duration_cost_function_raises_invalid_data():
    with assert_raises(ValueError):
        # At least one breakpoint and slope required.
        DurationCostFunction([], [])

    with assert_raises(ValueError):
        # First breakpoint must be 0.
        DurationCostFunction([1], [0])

    with assert_raises(ValueError):
        # Negative slope.
        DurationCostFunction([0], [-1])

    with assert_raises(ValueError):
        DurationCostFunction([0, 5], [2, 1])  # non-convex

    with assert_raises(ValueError):
        # Non-zero intercept.
        DurationCostFunction(PiecewiseLinearFunction([0], [0], intercept=1))


def test_duration_cost_function_constructs_from_piecewise_linear():
    pwl = PiecewiseLinearFunction([0, 5, 10], [2, 3, 4], intercept=0)
    duration_cost = DurationCostFunction(pwl)

    assert_equal(duration_cost.breakpoints, [0, 5, 10])
    assert_equal(duration_cost.slopes, [2, 3, 4])
    assert_equal(duration_cost.values, [0, 10, 25])
    assert_equal(duration_cost(13), 37)


def test_duration_cost_function_exposes_breakpoints_slopes_values():
    duration_cost = DurationCostFunction([0, 5, 10], [2, 3, 4])

    assert_equal(duration_cost.breakpoints, [0, 5, 10])
    assert_equal(duration_cost.slopes, [2, 3, 4])
    assert_equal(duration_cost.values, [0, 10, 25])
    assert_equal(duration_cost.edge_cost_slope, 2)
    assert_equal(duration_cost.piecewise_linear.values, [0, 10, 25])
    assert_equal(duration_cost(13), 37)


def test_duration_cost_function_is_zero():
    assert DurationCostFunction([0], [0]).is_zero()
    assert not DurationCostFunction([0], [1]).is_zero()


def test_duration_cost_function_equality():
    fn1 = DurationCostFunction([0, 5], [2, 3])
    fn2 = DurationCostFunction([0, 5], [2, 3])
    fn3 = DurationCostFunction([0, 5], [2, 4])

    assert fn1 == fn2
    assert fn1 != fn3


def test_duration_cost_function_raises_for_negative_duration():
    duration_cost = DurationCostFunction([0], [0])

    with assert_raises(ValueError, match="duration must be >= 0."):
        duration_cost(-1)

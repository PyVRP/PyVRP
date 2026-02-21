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


def test_duration_cost_function_raises_for_negative_duration():
    duration_cost = DurationCostFunction([0], [0])

    with assert_raises(ValueError, match="duration must be >= 0."):
        duration_cost(-1)


# def test_duration_cost_function_legacy_mapping_matches_definition():
#     vehicle_type = VehicleType(
#         shift_duration=10,
#         unit_duration_cost=2,
#         unit_overtime_cost=5,
#     )
#     duration_cost = vehicle_type.duration_cost_function

#     assert_equal(duration_cost(6), 12)  # 6 * 2 + 0 * 5
#     assert_equal(duration_cost(10), 20)  # 10 * 2 + 0 * 5
#     assert_equal(duration_cost(13), 41)  # 10 * 2 + 3 * (2 + 5) = 20 + 21
#     assert_equal(duration_cost.edge_cost_slope, 2)


# def test_duration_cost_function_legacy_mapping_with_zero_shift():
#     vehicle_type = VehicleType(
#         shift_duration=0,
#         unit_duration_cost=2,
#         unit_overtime_cost=5,
#     )
#     duration_cost = vehicle_type.duration_cost_function

#     # shift_duration is zero, so no new breakpoint is added.
#     assert_equal(duration_cost.breakpoints, [0])
#     # Overtime applies from the start: 2 + 5 = 7.
#     assert_equal(duration_cost.slopes, [7])
#     # All duration is overtime.
#     assert_equal(duration_cost(13), 2 * 13 + 5 * 13)


# def test_duration_cost_fn_max_shift_has_no_overtime_segment():
#     vehicle_type = VehicleType(
#         shift_duration=np.iinfo(np.int64).max,
#         unit_duration_cost=2,
#         unit_overtime_cost=5,
#     )
#     duration_cost = vehicle_type.duration_cost_function

#     # shift_duration is int64 max, so overtime never activates in
#     # int64 domain.
#     assert_equal(duration_cost.breakpoints, [0])
#     # Only the regular duration slope remains.
#     assert_equal(duration_cost.slopes, [2])
#     # All duration is regular time, no overtime.
#     assert_equal(duration_cost(13), 26)


# def test_duration_cost_fn_legacy_mapping_raises_on_slope_sum_overflow():
#     with assert_raises(
#         OverflowError,
#         match="unit_duration_cost \\+ unit_overtime_cost overflows.",
#     ):
#         VehicleType(
#             shift_duration=0,
#             unit_duration_cost=np.iinfo(np.int64).max,
#             unit_overtime_cost=1,
#         )


# def test_route_duration_cost_matches_vehicle_duration_cost_function():
#     data = ProblemData(
#         clients=[Client(x=0, y=1)],
#         depots=[Depot(x=0, y=0)],
#         vehicle_types=[
#             VehicleType(
#                 shift_duration=5,
#                 duration_cost_function=DurationCostFunction([0, 5], [1, 10]),
#             )
#         ],
#         distance_matrices=[np.array([[0, 0], [0, 0]], dtype=np.int64)],
#         duration_matrices=[np.array([[0, 4], [4, 0]], dtype=np.int64)],
#     )

#     route = Route(data, visits=[1], vehicle_type=0)
#     duration = route.duration()

#     # 4 from depot to client and 4 back.
#     assert_equal(duration, 8)
#     # 1 * 5 + 10 * 3 = 35 for duration 8 with breakpoints [0, 5].
#     assert_equal(route.duration_cost(), 35)
#     assert_equal(
#         route.duration_cost(),
#         data.vehicle_type(0).duration_cost_function(duration),
#     )


# def test_vehicle_type_raises_for_mixed_duration_cost_inputs():
#     with assert_raises(
#         ValueError,
#         match=(
#             "Provide either duration_cost_function or legacy "
#             "unit_duration_cost/unit_overtime_cost, not both."
#         ),
#     ):
#         VehicleType(
#             shift_duration=5,
#             unit_duration_cost=2,
#             unit_overtime_cost=5,
#             duration_cost_function=DurationCostFunction([0, 5], [1, 10]),
#         )


# def test_vehicle_type_replace_updates_legacy_duration_cost_function():
#     vehicle_type = VehicleType(
#         shift_duration=5,
#         unit_duration_cost=2,
#         unit_overtime_cost=3,
#     )

#     replaced = vehicle_type.replace(unit_duration_cost=4)
#     # 4 * 8 + 3 * (8 - 5) = 32 + 9 = 41.
#     assert_equal(replaced.duration_cost_function(8), 4 * 8 + 3 * 3)


# def test_vehicle_type_replace_rejects_legacy_update_for_custom_function():
#     custom = DurationCostFunction([0, 5], [1, 10])
#     vehicle_type = VehicleType(duration_cost_function=custom)

#     with assert_raises(
#         ValueError,
#         match=(
#             "Cannot update unit_duration_cost or unit_overtime_cost "
#             "when using "
#             "a custom duration_cost_function."
#         ),
#     ):
#         vehicle_type.replace(unit_duration_cost=7)


# def test_vehicle_type_replace_switches_to_legacy_after_clearing_custom():
#     custom = DurationCostFunction([0, 5], [1, 10])
#     vehicle_type = VehicleType(
#         shift_duration=5,
#         duration_cost_function=custom,
#     )

#     replaced = vehicle_type.replace(
#         duration_cost_function=None,
#         unit_duration_cost=4,
#         unit_overtime_cost=3,
#     )

#     assert_equal(replaced.duration_cost_function(8), 4 * 8 + 3 * 3)


# def test_vehicle_type_duration_cost_slope_delegates_to_duration_fn():
#     custom = DurationCostFunction([0, 5], [3, 10])
#     vehicle_type = VehicleType(duration_cost_function=custom)
#     assert_equal(vehicle_type.duration_cost_slope, 3)

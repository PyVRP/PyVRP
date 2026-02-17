import numpy as np
from numpy.testing import assert_equal
from pytest import raises as assert_raises

from pyvrp import (
    Client,
    Depot,
    DurationCostFunction,
    PiecewiseLinearFunction,
    ProblemData,
    Route,
    VehicleType,
)


def test_piecewise_linear_function_evaluates_segments():
    fn = PiecewiseLinearFunction([0, 5, 10], [2, 3, 4], intercept=1)

    assert_equal(fn(0), 1)      # 0 * 2 + 1
    assert_equal(fn(4), 9)      # 4 * 2 + 1
    assert_equal(fn(5), 11)     # 5 * 2 + 1
    assert_equal(fn(9), 23)     # 5 * 2 + 4 * 3 + 1
    assert_equal(fn(10), 26)    # 5 * 2 + 5 * 3 + 1
    assert_equal(fn(13), 38)    # 5 * 2 + 5 * 3 + 3 * 4 + 1


def test_piecewise_linear_function_raises_invalid_data():
    with assert_raises(ValueError):
        PiecewiseLinearFunction([], []) # at least one breakpoint and slope required

    with assert_raises(ValueError):
        PiecewiseLinearFunction([0, 1], [0]) # different number of breakpoints and slopes

    with assert_raises(ValueError):
        PiecewiseLinearFunction([0, 1, 1], [0, 0, 0]) # duplicate breakpoints


def test_piecewise_linear_function_raises_for_left_of_domain_query():
    fn = PiecewiseLinearFunction([5, 10], [2, 3], intercept=1)

    with assert_raises(ValueError, match="x must be >= first breakpoint."):
        fn(4)


def test_duration_cost_function_raises_invalid_data():
    with assert_raises(ValueError):
        DurationCostFunction([1], [0]) # first breakpoint must be 0

    with assert_raises(ValueError):
        DurationCostFunction([0], [-1]) # negative slope

    with assert_raises(ValueError):
        DurationCostFunction([0, 5], [2, 1])  # non-convex

    with assert_raises(ValueError):
        DurationCostFunction(PiecewiseLinearFunction([0], [0], intercept=1)) # non-zero intercept


def test_duration_cost_function_from_linear_matches_legacy_definition():
    duration_cost = DurationCostFunction.from_linear(
        shift_duration=10,
        unit_duration_cost=2,
        unit_overtime_cost=5,
    )

    assert_equal(duration_cost(6), 12)  # 6 * 2 + 0 * 5
    assert_equal(duration_cost(10), 20) # 10 * 2 + 0 * 5
    assert_equal(duration_cost(13), 41) # 10 * 2 + 3 * (2 + 5) = 20 + 21
    assert_equal(duration_cost.edge_cost_slope, 2)


def test_duration_cost_function_from_linear_with_zero_shift():
    duration_cost = DurationCostFunction.from_linear(
        shift_duration=0,
        unit_duration_cost=2,
        unit_overtime_cost=5,
    )

    assert_equal(duration_cost.breakpoints, [0])        # shift_duration is zero, so no new breakpoint is added
    assert_equal(duration_cost.slopes, [7])             # unit_duration_cost + unit_overtime_cost since overtime applies from the start
    assert_equal(duration_cost(13), 2 * 13 + 5 * 13)    # all duration is overtime


def test_duration_cost_function_from_linear_with_max_shift_has_no_overtime_segment():
    duration_cost = DurationCostFunction.from_linear(
        shift_duration=np.iinfo(np.int64).max,
        unit_duration_cost=2,
        unit_overtime_cost=5,
    )

    assert_equal(duration_cost.breakpoints, [0])        # shift_duration is max int64, so no new breakpoint is added within the domain of int64
    assert_equal(duration_cost.slopes, [2])             # unit_duration_cost only, since overtime would only apply after shift_duration which is beyond the int64 domain
    assert_equal(duration_cost(13), 26)                 # all duration is regular time, no overtime


def test_duration_cost_function_from_linear_raises_on_slope_sum_overflow():
    with assert_raises(
        OverflowError, match="unit_duration_cost \\+ unit_overtime_cost overflows."
    ):
        DurationCostFunction.from_linear(
            shift_duration=0,
            unit_duration_cost=np.iinfo(np.int64).max,
            unit_overtime_cost=1,
        )


def test_route_duration_cost_matches_vehicle_duration_cost_function():
    data = ProblemData(
        clients=[Client(x=0, y=1)],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[
            VehicleType(
                shift_duration=5,
                duration_cost_function=DurationCostFunction([0, 5], [1, 10]),
            )
        ],
        distance_matrices=[np.array([[0, 0], [0, 0]], dtype=np.int64)],
        duration_matrices=[np.array([[0, 4], [4, 0]], dtype=np.int64)],
    )

    route = Route(data, visits=[1], vehicle_type=0)
    duration = route.duration()

    assert_equal(duration, 8)                   # 4 from depot to client and 4 back
    assert_equal(route.duration_cost(), 35)     # 1 * 5 + 10 * 3 = 35 for duration 8 with breakpoints [0, 5] and slopes [1, 10].
    assert_equal(
        route.duration_cost(),
        data.vehicle_type(0).duration_cost_function(duration),
    ) # the route's duration cost should match the vehicle type's duration cost function evaluated at the route duration


def test_vehicle_type_raises_for_mixed_custom_and_legacy_duration_cost_inputs():
    with assert_raises(
        ValueError,
        match=(
            "Provide either duration_cost_function or legacy "
            "unit_duration_cost/unit_overtime_cost, not both."
        ),
    ):
        VehicleType(
            shift_duration=5,
            unit_duration_cost=2,
            unit_overtime_cost=5,
            duration_cost_function=DurationCostFunction([0, 5], [1, 10]),
        )


def test_vehicle_type_replace_updates_legacy_duration_cost_function():
    vehicle_type = VehicleType(
        shift_duration=5,
        unit_duration_cost=2,
        unit_overtime_cost=3,
    )

    replaced = vehicle_type.replace(unit_duration_cost=4)
    assert_equal(replaced.duration_cost_function(8), 4 * 8 + 3 * 3) # 4 * 8 + 3 * (8 - 5) = 32 + 9 = 41 because the new unit_duration_cost is 4 and the overtime cost applies to the duration beyond the shift_duration of 5


def test_vehicle_type_replace_raises_when_updating_legacy_cost_for_custom_function():
    custom = DurationCostFunction([0, 5], [1, 10])
    vehicle_type = VehicleType(duration_cost_function=custom)

    with assert_raises(
        ValueError,
        match=(
            "Cannot update unit_duration_cost or unit_overtime_cost when using "
            "a custom duration_cost_function."
        ),
    ):
        vehicle_type.replace(unit_duration_cost=7)


def test_vehicle_type_replace_switches_to_legacy_when_duration_cost_is_cleared():
    custom = DurationCostFunction([0, 5], [1, 10])
    vehicle_type = VehicleType(
        shift_duration=5,
        duration_cost_function=custom,
    )

    replaced = vehicle_type.replace(
        duration_cost_function=None,
        unit_duration_cost=4,
        unit_overtime_cost=3,
    )

    assert_equal(replaced.duration_cost_function(8), 4 * 8 + 3 * 3)


def test_vehicle_type_duration_cost_slope_delegates_to_duration_cost_function():
    custom = DurationCostFunction([0, 5], [3, 10])
    vehicle_type = VehicleType(duration_cost_function=custom)
    assert_equal(vehicle_type.duration_cost_slope, 3)

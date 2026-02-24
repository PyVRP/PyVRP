import numpy as np
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import (
    Client,
    Depot,
    PiecewiseLinearFunction,
    ProblemData,
    Route,
    VehicleType,
)

_INT_MAX = int(np.iinfo(np.int64).max)


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


def test_vehicle_type_uses_provided_duration_cost_function():
    duration_fn = PiecewiseLinearFunction(
        [5, _INT_MAX],
        [(0, 1), (-45, 10)],
    )
    vehicle_type = VehicleType(duration_cost_function=duration_fn)

    assert_equal(vehicle_type.duration_cost_function, duration_fn)
    assert_equal(vehicle_type.duration_cost_slope, 1)


def test_vehicle_type_defaults_to_zero_duration_cost_function():
    vehicle_type = VehicleType()
    assert vehicle_type.duration_cost_function.is_zero()


def test_route_duration_cost_matches_vehicle_duration_cost_function():
    duration_fn = PiecewiseLinearFunction([5, _INT_MAX], [(0, 1), (-45, 10)])

    data = ProblemData(
        clients=[Client(x=0, y=1)],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(duration_cost_function=duration_fn)],
        distance_matrices=[np.array([[0, 0], [0, 0]], dtype=np.int64)],
        duration_matrices=[np.array([[0, 4], [4, 0]], dtype=np.int64)],
    )

    route = Route(data, visits=[1], vehicle_type=0)
    duration = route.duration()

    assert_equal(duration, 8)  # 4 from depot to client and 4 back.
    assert_equal(route.duration_cost(), 35)  # 5 + 10 * (8 - 5)
    assert_equal(
        route.duration_cost(), data.vehicle_type(0).duration_cost_function(8)
    )


def test_vehicle_type_replace_updates_duration_cost_function():
    original = PiecewiseLinearFunction([0], [(0, 1)])
    updated = PiecewiseLinearFunction([5, _INT_MAX], [(0, 1), (-45, 10)])
    vehicle_type = VehicleType(duration_cost_function=original)

    replaced = vehicle_type.replace(duration_cost_function=updated)
    unchanged = vehicle_type.replace()

    assert_equal(replaced.duration_cost_function, updated)
    assert_equal(unchanged.duration_cost_function, original)

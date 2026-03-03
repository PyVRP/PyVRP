import pickle

import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import PiecewiseLinearFunction


def test_call_from_slope_points():
    """
    Tests calling the piecewise linear function on various segments.
    """
    fn = PiecewiseLinearFunction(points=[(0, 2), (5, 3), (10, 4, 15)])

    # 1st segment, defined for values < 5.
    assert_equal(fn(-100), 2 * -100)
    assert_equal(fn(0), 0)
    assert_equal(fn(4), 2 * 4)

    # 2nd segment, defined for values in [5, 10).
    assert_equal(fn(5), 10)
    assert_equal(fn(9), 22)

    # 3rd segment, defined for values >= 10.
    assert_equal(fn(10), 40)
    assert_equal(fn(13), 52)
    assert_equal(fn(100), 400)


def test_call_from_breakpoints_and_segments():
    """
    Tests calling the piecewise linear function using the `breakpoints`
    and `segments` constructor.
    """
    fn = PiecewiseLinearFunction(
        breakpoints=[5, 10], segments=[(1, 2), (11, 3), (26, 4)]
    )

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


def test_breakpoints_and_segments_properties_from_points_constructor():
    """
    Tests internal breakpoints/segments properties from points constructor.
    """
    fn = PiecewiseLinearFunction(points=[(0, 1), (5, 10, 3)])
    assert_equal(fn.breakpoints, [0, 5])
    assert_equal(fn.segments, [(0, 1), (0, 1), (-42, 10)])


def test_breakpoints_and_segments_properties_from_legacy_constructor():
    """
    Tests breakpoints/segments properties from legacy constructor.
    """
    breakpoints = [0, 5]
    segments = [(0, 1), (0, 1), (-42, 10)]
    fn = PiecewiseLinearFunction(breakpoints=breakpoints, segments=segments)

    assert_equal(fn.breakpoints, breakpoints)
    assert_equal(fn.segments, segments)


def test_jump_at_final_breakpoint_from_breakpoints_and_segments():
    """
    Tests a jump at the final breakpoint using n breakpoints
    and n + 1 segments.
    """
    fn = PiecewiseLinearFunction(
        breakpoints=[0, 5], segments=[(0, 1), (0, 1), (3, 1)]
    )

    assert_equal(fn(4), 4)  # left of final breakpoint
    assert_equal(fn(5), 8)  # at final breakpoint (jump of +3)
    assert_equal(fn(6), 9)  # right of final breakpoint


def test_call_from_single_point():
    """
    Tests constructing from a single point, yielding a constant function.
    """
    fn = PiecewiseLinearFunction(points=[(3, 0, 7)])
    assert_equal(fn(-10), 7)
    assert_equal(fn(3), 7)
    assert_equal(fn(100), 7)


def test_call_from_single_breakpoint_with_jump():
    """
    Tests the optional jump on a single breakpoint.
    """
    fn = PiecewiseLinearFunction(points=[(3, 2, 5)])
    assert_equal(fn(2), 3)
    assert_equal(fn(3), 5)
    assert_equal(fn(100), 199)


def test_jump_applies_at_and_to_the_right():
    """
    Tests that jump values apply at and to the right of the breakpoint.
    """
    fn = PiecewiseLinearFunction(points=[(0, 1), (5, 1, 2), (10, 1, 3)])

    # Left of x = 5, we are still on the left-hand segment.
    assert_equal(fn(4), 4)

    # At and right of x = 5, we use the value including the jump.
    assert_equal(fn(5), 7)
    assert_equal(fn(9), 11)
    assert_equal(fn(10), 15)


def test_multiple_jumps_prefer_left_side_values():
    """
    Tests left-side behaviour for multiple jump breakpoints.
    """
    fn = PiecewiseLinearFunction(points=[(0, 1), (3, 1, 7), (6, 2, 5)])

    # First jump at x = 3.
    assert_equal(fn(2), 2)  # left side
    assert_equal(fn(3), 10)  # right side

    # Second jump at x = 6.
    assert_equal(fn(5), 12)  # left side
    assert_equal(fn(6), 18)  # right side


def test_zero():
    """
    Tests the piecewise linear function with zero slope and/or jump.
    """
    fn = PiecewiseLinearFunction(points=[(0, 7)])  # zero jump
    assert_equal(fn(-1), -7)
    assert_equal(fn(0), 0)
    assert_equal(fn(1), 7)

    fn = PiecewiseLinearFunction(points=[(0, 0, 7)])  # zero slope (constant)
    assert_equal(fn(-5), 7)
    assert_equal(fn(0), 7)
    assert_equal(fn(5), 7)

    fn = PiecewiseLinearFunction(points=[(0, 0)])  # all zero
    assert_equal(fn(-100), 0)
    assert_equal(fn(0), 0)
    assert_equal(fn(100), 0)


def test_default_constructor():
    """
    Tests the default constructor, which should be the zero function.
    """
    fn = PiecewiseLinearFunction()
    assert_equal(fn(-5), 0)
    assert_equal(fn(0), 0)
    assert_equal(fn(5), 0)


def test_raises_invalid_points():
    """
    Tests invalid points input.
    """
    with assert_raises(ValueError):  # at least one point is needed
        PiecewiseLinearFunction([])

    with assert_raises(ValueError):  # x must be strictly increasing
        PiecewiseLinearFunction(points=[(1, 2), (1, 3)])

    with assert_raises(ValueError):  # x must be strictly increasing
        PiecewiseLinearFunction(points=[(1, 2), (0, 3)])

    with assert_raises(ValueError):  # wrong tuple length
        PiecewiseLinearFunction(points=[(0, 0, 1, 2)])


def test_raises_invalid_breakpoints_and_segments():
    """
    Tests invalid input for the legacy constructor.
    """
    with assert_raises(ValueError):  # need at least one segment
        PiecewiseLinearFunction(breakpoints=[], segments=[])

    with assert_raises(ValueError):  # need one more segment than breakpoints
        PiecewiseLinearFunction(breakpoints=[0], segments=[])

    with assert_raises(ValueError):  # need one more segment than breakpoints
        PiecewiseLinearFunction(breakpoints=[0], segments=[(0, 0)])

    with assert_raises(ValueError):  # need one more segment than breakpoints
        PiecewiseLinearFunction(breakpoints=[], segments=[(0, 0), (0, 0)])

    with assert_raises(ValueError):  # breakpoints must be strictly increasing
        PiecewiseLinearFunction(breakpoints=[1, 1], segments=[(1, 1), (2, 2)])

    with assert_raises(ValueError):  # breakpoints must be strictly increasing
        PiecewiseLinearFunction(breakpoints=[2, 1], segments=[(1, 1), (2, 2)])


def test_getstate_returns_internal_representation():
    """
    Tests getting the serialised internal representation.
    """
    fn = PiecewiseLinearFunction(points=[(0, 1), (5, 10, 3)])
    assert_equal(
        fn.__getstate__(),
        ([0, 5], [(0, 1), (0, 1), (-42, 10)]),
    )


def test_pickle_roundtrip_from_points_constructor():
    """
    Tests round-trip serialisation and deserialisation.
    """
    fn = PiecewiseLinearFunction(points=[(0, 1), (5, 10, 3)])
    fn2 = pickle.loads(pickle.dumps(fn, protocol=4))
    assert_equal(fn, fn2)


def test_pickle_roundtrip_from_breakpoints_and_segments_constructor():
    """
    Tests round-trip serialisation from the breakpoints/segments constructor.
    """
    fn = PiecewiseLinearFunction(
        breakpoints=[0, 5], segments=[(0, 1), (0, 1), (-42, 10)]
    )
    fn2 = pickle.loads(pickle.dumps(fn, protocol=4))
    assert_equal(fn, fn2)


@pytest.mark.parametrize(
    ("points", "expected"),
    [
        ([(0, 1), (5, 1, 3), (10, 2)], True),  # non-decreasing
        ([(0, -1)], False),  # negative slope
        ([(0, 2), (5, 2, -3)], False),  # downward jump
    ],
    ids=["non_decreasing", "negative_slope", "downward_jump"],
)
def test_is_monotonically_increasing_for_both_constructors(
    points: list[tuple[int, int] | tuple[int, int, int]],
    expected: bool,
):
    """
    Tests monotonicity checks for both constructors.
    """
    from_points = PiecewiseLinearFunction(points=points)
    from_breakpoints_segments = PiecewiseLinearFunction(
        breakpoints=from_points.breakpoints,
        segments=from_points.segments,
    )

    assert_equal(from_points.is_monotonically_increasing(), expected)
    assert_equal(
        from_breakpoints_segments.is_monotonically_increasing(),
        expected,
    )


def test_eq():
    """
    Tests equality comparisons between various functions.
    """
    fn1 = PiecewiseLinearFunction(points=[(0, 2), (1, 3, 4)])
    fn2 = PiecewiseLinearFunction(points=[(0, 2), (1, 3, 4)])
    fn3 = PiecewiseLinearFunction(points=[(0, 2), (1, 4, 4)])
    fn4 = PiecewiseLinearFunction(points=[(1, 2), (2, 3, 4)])

    assert_(fn1 == fn2)
    assert_(fn1 != fn3)
    assert_(fn1 != fn4)
    assert_(fn1 != "string")

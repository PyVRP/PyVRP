import pickle

import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import PiecewiseLinearFunction


def test_call_from_points():
    """
    Tests calling the piecewise linear function on various segments.
    Gurobi-style (x, f(x)) interface: slope 2 before x=5, slope 3 from x=5
    to x=10, then a jump of 15 at x=10, then slope 4 from x=10 onward.
    """
    # f(0)=0, f(5)=10, f(10-)=25, f(10)=40 (jump), slope 4 after x=10.
    fn = PiecewiseLinearFunction(
        points=[(0, 0), (5, 10), (10, 25), (10, 40), (11, 44)]
    )

    # Left extrapolation: slope 2.
    assert_equal(fn(-100), 2 * -100)
    assert_equal(fn(0), 0)
    assert_equal(fn(4), 2 * 4)

    # Second segment: slope 3, for x in [5, 10).
    assert_equal(fn(5), 10)
    assert_equal(fn(9), 22)

    # Third segment: slope 4, starting at (10, 40).
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
    Tests internal breakpoints/segments properties from the points constructor.

    For points=[(0, 0), (5, 5), (5, 8), (6, 18)]: slope 1 before x=5, jump
    from 5 to 8 at x=5, slope 10 after x=5. All input x-values become
    internal breakpoints.
    """
    fn = PiecewiseLinearFunction(points=[(0, 0), (5, 5), (5, 8), (6, 18)])
    # All distinct x-values [0, 5, 6] become breakpoints.
    assert_equal(fn.breakpoints, [0, 5, 6])
    # Left-extrapolation segment (slope 1), then [0,5): slope 1, [5,6): slope
    # 10 (intercept -42), [6,inf): slope 10 (same right extrapolation).
    assert_equal(fn.segments, [(0, 1), (0, 1), (-42, 10), (-42, 10)])


def test_breakpoints_and_segments_properties():
    """
    Tests breakpoints/segments properties from the breakpoints and segments
    constructor.
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


def test_call_from_two_point():
    """
    Tests constructing from two points, yielding a single linear segment.
    """
    fn = PiecewiseLinearFunction(points=[(0, 0), (1, 7)])
    assert_equal(fn(-1), -7)
    assert_equal(fn(0), 0)
    assert_equal(fn(1), 7)
    assert_equal(fn(2), 14)


def test_call_constant_function():
    """
    Tests a constant function (zero slope, same y on both endpoints).
    """
    fn = PiecewiseLinearFunction(points=[(0, 7), (1, 7)])
    assert_equal(fn(-5), 7)
    assert_equal(fn(0), 7)
    assert_equal(fn(5), 7)


def test_jump_applies_at_and_to_the_right():
    """
    Tests that jump values apply at and to the right of the breakpoint.
    """
    # Slope 1, jump +2 at x=5, slope 1, jump +3 at x=10, slope 1.
    fn = PiecewiseLinearFunction(
        points=[(0, 0), (5, 5), (5, 7), (10, 12), (10, 15), (11, 16)]
    )

    # Left of x=5: slope 1.
    assert_equal(fn(4), 4)

    # At and right of x=5: value includes the jump.
    assert_equal(fn(5), 7)
    assert_equal(fn(9), 11)
    assert_equal(fn(10), 15)


def test_multiple_jumps():
    """
    Tests left-side and right-side values for multiple jump breakpoints.
    """
    # Slope 1, jump +7 at x=3, slope 1, jump +5 at x=6, slope 2.
    fn = PiecewiseLinearFunction(
        points=[(0, 0), (3, 3), (3, 10), (6, 13), (6, 18), (7, 20)]
    )

    # First jump at x=3.
    assert_equal(fn(2), 2)  # left side
    assert_equal(fn(3), 10)  # right side (jump of +7)

    # Second jump at x=6.
    assert_equal(fn(5), 12)  # left side
    assert_equal(fn(6), 18)  # right side (jump of +5)


def test_zero():
    """
    Tests the piecewise linear function with zero slope and/or jump.
    """
    fn = PiecewiseLinearFunction(points=[(0, 0), (1, 7)])  # slope 7
    assert_equal(fn(-1), -7)
    assert_equal(fn(0), 0)
    assert_equal(fn(1), 7)

    fn = PiecewiseLinearFunction(  # zero slope (constant)
        points=[(0, 7), (1, 7)]
    )
    assert_equal(fn(-5), 7)
    assert_equal(fn(0), 7)
    assert_equal(fn(5), 7)

    fn = PiecewiseLinearFunction(points=[(0, 0), (1, 0)])  # all zero
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
    with assert_raises(ValueError):  # need at least two points
        PiecewiseLinearFunction([])

    with assert_raises(ValueError):  # need at least two points
        PiecewiseLinearFunction(points=[(1, 2)])

    with assert_raises(ValueError):  # x must be non-decreasing
        PiecewiseLinearFunction(points=[(1, 2), (0, 3)])

    with assert_raises(ValueError):  # jump at first position
        PiecewiseLinearFunction(points=[(1, 2), (1, 3), (2, 5)])

    with assert_raises(ValueError):  # jump at last position
        PiecewiseLinearFunction(points=[(0, 0), (1, 2), (1, 3)])

    with assert_raises(ValueError):  # non-integral slope
        PiecewiseLinearFunction(points=[(0, 0), (3, 1)])

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


def test_raises_unsorted_breakpoints():
    """
    Tests unsorted and non-strictly increasing breakpoints.
    """
    with assert_raises(ValueError):  # unsorted
        PiecewiseLinearFunction(
            breakpoints=[2, 1, 3], segments=[(1, 1), (2, 2), (3, 3), (4, 4)]
        )

    with assert_raises(ValueError):  # not strictly increasing
        PiecewiseLinearFunction(
            breakpoints=[1, 1], segments=[(1, 1), (2, 2), (3, 3)]
        )


def test_getstate_returns_internal_representation():
    """
    Tests getting the serialised internal representation.

    The points constructor adds every distinct input x as an internal
    breakpoint, so all of 0, 5, 6 appear in the breakpoint list.
    """
    fn = PiecewiseLinearFunction(points=[(0, 0), (5, 5), (5, 8), (6, 18)])
    assert_equal(
        fn.__getstate__(),
        ([0, 5, 6], [(0, 1), (0, 1), (-42, 10), (-42, 10)]),
    )


def test_pickle_roundtrip_from_points_constructor():
    """
    Tests round-trip serialisation and deserialisation.
    """
    fn = PiecewiseLinearFunction(points=[(0, 0), (5, 5), (5, 8), (6, 18)])
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
        # Slope 1 everywhere with a +3 upward jump at x=5 and slope 2 after
        # x=10: non-decreasing throughout.
        ([(0, 0), (5, 5), (5, 8), (10, 13), (11, 15)], True),
        # Negative slope: not monotonically increasing.
        ([(0, 0), (1, -1)], False),
        # Downward jump at x=5: not monotonically increasing.
        ([(0, 0), (5, 10), (5, 7), (6, 9)], False),
    ],
    ids=["non_decreasing", "negative_slope", "downward_jump"],
)
def test_is_monotonically_increasing_for_both_constructors(
    points: list[tuple[int, int]],
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
    # slope 2, jump +4 at x=1, slope 3.
    fn1 = PiecewiseLinearFunction(points=[(0, 0), (1, 2), (1, 6), (2, 9)])
    fn2 = PiecewiseLinearFunction(points=[(0, 0), (1, 2), (1, 6), (2, 9)])
    # slope 2, jump +4 at x=1, slope 4.
    fn3 = PiecewiseLinearFunction(points=[(0, 0), (1, 2), (1, 6), (2, 10)])
    # same shape but shifted: slope 2 starting at x=1.
    fn4 = PiecewiseLinearFunction(points=[(1, 0), (2, 2), (2, 6), (3, 9)])

    assert_(fn1 == fn2)
    assert_(fn1 != fn3)
    assert_(fn1 != fn4)
    assert_(fn1 != "string")

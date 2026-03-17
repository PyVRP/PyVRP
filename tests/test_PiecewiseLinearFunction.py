import pickle

import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import PiecewiseLinearFunction

# ---------------------------------------------------------------------------
# 1. Validation
# ---------------------------------------------------------------------------


def test_raises_invalid_points():
    """
    Tests that the points constructor raises ValueError for invalid inputs.
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

    with assert_raises((ValueError, TypeError)):  # wrong tuple length
        PiecewiseLinearFunction(points=[(0, 0, 1, 2)])


def test_raises_invalid_breakpoints_and_segments():
    """
    Tests that the breakpoints/segments constructor raises ValueError for
    invalid inputs.
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


# ---------------------------------------------------------------------------
# 2. Constructor equivalence
# ---------------------------------------------------------------------------


def test_constructors_are_equivalent():
    """
    Tests that both constructors produce equal objects for the same function.

    For points=[(0, 0), (5, 5), (5, 8), (6, 18)]: slope 1 before x=5, jump
    from 5 to 8 at x=5, slope 10 after x=5. The expected internal
    representation is breakpoints=[0, 5, 6] and segments as below.
    """
    from_points = PiecewiseLinearFunction(
        points=[(0, 0), (5, 5), (5, 8), (6, 18)]
    )
    from_breakpoints_segments = PiecewiseLinearFunction(
        breakpoints=[0, 5, 6],
        segments=[(0, 1), (0, 1), (-42, 10), (-42, 10)],
    )

    assert_(from_points == from_breakpoints_segments)


# ---------------------------------------------------------------------------
# 3. Properties
# ---------------------------------------------------------------------------


def test_properties():
    """
    Tests that the points constructor correctly maps (x, f(x)) pairs to the
    internal breakpoints/segments representation, and that __getstate__
    exposes the same data.

    For points=[(0, 0), (5, 5), (5, 8), (6, 18)]: all distinct x-values
    [0, 5, 6] become breakpoints. The left-extrapolation segment (slope 1)
    and each subsequent segment are derived from the point pairs.
    """
    fn = PiecewiseLinearFunction(points=[(0, 0), (5, 5), (5, 8), (6, 18)])

    assert_equal(fn.breakpoints, [0, 5, 6])
    assert_equal(fn.segments, [(0, 1), (0, 1), (-42, 10), (-42, 10)])
    assert_equal(
        fn.__getstate__(),
        ([0, 5, 6], [(0, 1), (0, 1), (-42, 10), (-42, 10)]),
    )


# ---------------------------------------------------------------------------
# 4. Evaluation
# ---------------------------------------------------------------------------


def test_call():
    """
    Tests evaluating a multi-segment function, including left and right
    extrapolation beyond the defined points.

    slope 2 before x=5, slope 3 from x=5 to x=10, jump of 15 at x=10,
    slope 4 from x=10 onward.
    """
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

    # Right extrapolation: slope 4.
    assert_equal(fn(100), 400)


def test_call_constant_function():
    """
    Tests evaluating a constant (zero-slope) function.
    """
    fn = PiecewiseLinearFunction(points=[(0, 7), (1, 7)])
    assert_equal(fn(-5), 7)
    assert_equal(fn(0), 7)
    assert_equal(fn(5), 7)


# ---------------------------------------------------------------------------
# 5. Jump semantics
# ---------------------------------------------------------------------------


def test_jump_applies_at_and_to_the_right():
    """
    Tests that a jump value applies at and to the right of the breakpoint.

    slope 1, jump +2 at x=5, slope 1, jump +3 at x=10, slope 1.
    """
    fn = PiecewiseLinearFunction(
        points=[(0, 0), (5, 5), (5, 7), (10, 12), (10, 15), (11, 16)]
    )

    assert_equal(fn(4), 4)  # left of first jump
    assert_equal(fn(5), 7)  # at first jump: right-side value applies
    assert_equal(fn(9), 11)  # between jumps
    assert_equal(fn(10), 15)  # at second jump: right-side value applies


# ---------------------------------------------------------------------------
# 6. Monotonicity
# ---------------------------------------------------------------------------


@pytest.mark.parametrize(
    ("points", "expected"),
    [
        # Upward jump at x=5 and slope 2 after x=10: non-decreasing throughout.
        ([(0, 0), (5, 5), (5, 8), (10, 13), (11, 15)], True),
        # Negative slope: not monotonically increasing.
        ([(0, 0), (1, -1)], False),
        # Downward jump at x=5: not monotonically increasing.
        ([(0, 0), (5, 10), (5, 7), (6, 9)], False),
    ],
    ids=["upward_jump", "negative_slope", "downward_jump"],
)
def test_is_monotonically_increasing(points, expected):
    """
    Tests is_monotonically_increasing for an upward-jump, negative-slope,
    and downward-jump case.
    """
    fn = PiecewiseLinearFunction(points=points)
    assert_equal(fn.is_monotonically_increasing(), expected)


# ---------------------------------------------------------------------------
# 7. Equality
# ---------------------------------------------------------------------------


def test_eq():
    """
    Tests equality comparisons between various functions.
    """
    # slope 2, jump +4 at x=1, slope 3.
    fn1 = PiecewiseLinearFunction(points=[(0, 0), (1, 2), (1, 6), (2, 9)])
    fn2 = PiecewiseLinearFunction(points=[(0, 0), (1, 2), (1, 6), (2, 9)])
    # slope 2, jump +4 at x=1, slope 4 (different right slope).
    fn3 = PiecewiseLinearFunction(points=[(0, 0), (1, 2), (1, 6), (2, 10)])
    # same shape but shifted: slope 2 starting at x=1.
    fn4 = PiecewiseLinearFunction(points=[(1, 0), (2, 2), (2, 6), (3, 9)])

    assert_(fn1 == fn2)
    assert_(fn1 != fn3)
    assert_(fn1 != fn4)
    assert_(fn1 != "string")


# ---------------------------------------------------------------------------
# 8. Pickle roundtrip
# ---------------------------------------------------------------------------


def test_pickle_roundtrip():
    """
    Tests round-trip serialisation and deserialisation via __getstate__ and
    __setstate__.
    """
    fn = PiecewiseLinearFunction(points=[(0, 0), (5, 5), (5, 8), (6, 18)])
    fn2 = pickle.loads(pickle.dumps(fn, protocol=4))
    assert_equal(fn, fn2)

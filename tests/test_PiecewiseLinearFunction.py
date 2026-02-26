from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import PiecewiseLinearFunction


def test_call_from_points():
    """
    Tests calling the piecewise linear function on various segments.
    """
    fn = PiecewiseLinearFunction(
        [
            (0, 1),
            (5, 11),
            (5, 26),
            (10, 41),
            (10, 66),
            (11, 70),
        ]
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


def test_call_from_single_point():
    """
    Tests constructing from a single point, yielding a constant function.
    """
    fn = PiecewiseLinearFunction([(3, 7)])
    assert_equal(fn(-10), 7)
    assert_equal(fn(3), 7)
    assert_equal(fn(100), 7)


def test_call_from_single_x_with_jump():
    """
    Tests repeated x-values, which define jumps.
    """
    fn = PiecewiseLinearFunction([(3, 7), (3, 9)])
    assert_equal(fn(2), 7)
    assert_equal(fn(3), 9)
    assert_equal(fn(100), 9)


def test_jump_prefers_left_side_value_before_repeated_x():
    """
    Tests that values immediately left of a repeated x use the left side.
    """
    fn = PiecewiseLinearFunction([(0, 0), (5, 5), (5, 7), (5, 9), (10, 14)])

    # Left of x = 5, we are still on the left-hand segment.
    assert_equal(fn(4), 4)

    # At and right of x = 5, we use the last value at that x.
    assert_equal(fn(5), 9)
    assert_equal(fn(6), 10)


def test_multiple_jumps_prefer_left_side_values():
    """
    Tests left-side behaviour for multiple repeated x-value jumps.
    """
    fn = PiecewiseLinearFunction(
        [(0, 0), (2, 2), (2, 10), (4, 12), (4, 20), (6, 22)]
    )

    # First jump at x = 2.
    assert_equal(fn(1), 1)  # left side
    assert_equal(fn(2), 10)  # right side

    # Second jump at x = 4.
    assert_equal(fn(3), 11)  # left side
    assert_equal(fn(4), 20)  # right side


def test_repeated_x_jumps_use_left_value_before_and_last_value_at_x():
    """
    Tests left-of-jump and at-jump values for repeated x-groups.
    """
    fn = PiecewiseLinearFunction(
        [(0, 0), (3, 3), (3, 8), (3, 10), (6, 16), (6, 20)]
    )

    # First jump at x = 3.
    assert_equal(fn(2), 2)  # left side
    assert_equal(fn(3), 10)  # right side (last y at x = 3)

    # Second jump at x = 6.
    assert_equal(fn(5), 14)  # left side
    assert_equal(fn(6), 20)  # right side (last y at x = 6)


def test_zero():
    """
    Tests the piecewise linear function with zero slope and/or intercept.
    """
    fn = PiecewiseLinearFunction([(0, 0), (1, 7)])  # zero intercept
    assert_equal(fn(-1), -7)
    assert_equal(fn(0), 0)
    assert_equal(fn(1), 7)

    fn = PiecewiseLinearFunction([(0, 7)])  # zero slope (constant)
    assert_equal(fn(-5), 7)
    assert_equal(fn(0), 7)
    assert_equal(fn(5), 7)

    fn = PiecewiseLinearFunction([(0, 0)])  # all zero
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
    Tests invalid (x, y) points input.
    """
    with assert_raises(ValueError):  # at least one point is needed
        PiecewiseLinearFunction([])

    with assert_raises(ValueError):  # x must be non-decreasing
        PiecewiseLinearFunction([(1, 2), (0, 3)])

    with assert_raises(ValueError):  # slopes must be integer-valued
        PiecewiseLinearFunction([(0, 0), (2, 3)])


def test_getstate_returns_internal_representation():
    """
    Tests getting the serialised internal representation.
    """
    fn = PiecewiseLinearFunction([(0, 0), (5, 5), (10, 55)])
    assert_equal(
        fn.__getstate__(),
        ([0, 5, 10, 10], [(0, 1), (0, 1), (-45, 10), (-45, 10)]),
    )


def test_eq():
    """
    Tests equality comparisons between various functions.
    """
    fn1 = PiecewiseLinearFunction([(0, 0), (1, 2)])
    fn2 = PiecewiseLinearFunction([(0, 0), (1, 2)])
    fn3 = PiecewiseLinearFunction([(0, 0), (1, 3)])
    fn4 = PiecewiseLinearFunction([(1, 0), (2, 2)])

    assert_(fn1 == fn2)
    assert_(fn1 != fn3)
    assert_(fn1 != fn4)
    assert_(fn1 != "string")

import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import BatterySegment

_INT_MAX = np.iinfo(np.int64).max


def test_default_values():
    """
    Tests that a default-constructed segment is empty and has no charger.
    """
    segment = BatterySegment()
    assert_equal(segment.head(), 0)
    assert_equal(segment.tail(), 0)
    assert_(not segment.charges())
    assert_equal(segment.excess_charge(0), 0)


def test_attribute_getters():
    """
    Tests that the attribute member functions return the passed in values.
    """
    segment = BatterySegment(head=1, tail=2, excess_charge=3, charges=True)
    assert_equal(segment.head(), 1)
    assert_equal(segment.tail(), 2)
    assert_(segment.charges())
    assert_equal(segment.excess_charge(10), 3)


@pytest.mark.parametrize(
    ("first", "second", "exp_head", "exp_tail", "exp_charges", "exp_excess"),
    [
        # No chargers: there is one stretch of 3 + 5 + 7 = 15, so 5 in excess.
        (BatterySegment(3, 3), BatterySegment(7, 7), 15, 15, False, 5),
        # Charger in the first segment, so the second extends the first's tail.
        (
            BatterySegment(4, 3, charges=True),
            BatterySegment(7, 7),
            4,
            15,
            True,
            5,
        ),
        # Charger in the second segment, so the first extends the second's
        # head.
        (
            BatterySegment(3, 3),
            BatterySegment(7, 4, charges=True),
            15,
            4,
            True,
            5,
        ),
        # Chargers in both segments. The stretch between them is 15, which
        # exceeds the capacity by 5. The head and tail do not.
        (
            BatterySegment(1, 3, charges=True),
            BatterySegment(7, 2, charges=True),
            1,
            2,
            True,
            5,
        ),
        # As before, but both segments also bring in their own excess.
        (
            BatterySegment(1, 3, excess_charge=20, charges=True),
            BatterySegment(7, 2, excess_charge=30, charges=True),
            1,
            2,
            True,
            55,
        ),
    ],
)
def test_merge(
    first: BatterySegment,
    second: BatterySegment,
    exp_head: int,
    exp_tail: int,
    exp_charges: bool,
    exp_excess: int,
):
    """
    Tests that merging two segments, connected by an edge of distance 5,
    correctly tracks the stretches between chargers.
    """
    merged = BatterySegment.merge(5, first, second, 10)
    assert_equal(merged.head(), exp_head)
    assert_equal(merged.tail(), exp_tail)
    assert_equal(merged.charges(), exp_charges)
    assert_equal(merged.excess_charge(10), exp_excess)


def test_merge_is_associative():
    """
    Tests that the order in which segments are merged does not matter.
    """
    capacity = 10
    edges = [4, 8, 2]
    segments = [
        BatterySegment(3, 3),
        BatterySegment(0, 0, charges=True),
        BatterySegment(9, 9),
        BatterySegment(0, 0, charges=True),
    ]

    merge = BatterySegment.merge
    left = merge(edges[0], segments[0], segments[1], capacity)
    left = merge(edges[1], left, segments[2], capacity)
    left = merge(edges[2], left, segments[3], capacity)

    right = merge(edges[2], segments[2], segments[3], capacity)
    right = merge(edges[1], segments[1], right, capacity)
    right = merge(edges[0], segments[0], right, capacity)

    assert_equal(left.head(), right.head())
    assert_equal(left.tail(), right.tail())
    assert_equal(left.charges(), right.charges())
    assert_equal(left.excess_charge(capacity), right.excess_charge(capacity))

    # First stretch is 3 + 4 = 7, which fits. The second is 8 + 9 + 2 = 19,
    # which exceeds the capacity by 9.
    assert_equal(left.excess_charge(capacity), 9)


@pytest.mark.parametrize(
    ("segment", "capacity", "expected"),
    [
        (BatterySegment(10, 10), 10, 0),  # exactly fits
        (BatterySegment(10, 10), 9, 1),  # head and tail are the same stretch
        (BatterySegment(10, 4, charges=True), 5, 5),  # only head is in excess
        (BatterySegment(4, 10, charges=True), 5, 5),  # only tail is in excess
        (BatterySegment(9, 9, charges=True), 5, 8),  # both are in excess
        (BatterySegment(10, 10), _INT_MAX, 0),  # unconstrained
    ],
)
def test_excess_charge(segment: BatterySegment, capacity: int, expected: int):
    """
    Tests that the excess charge counts the head and tail stretches, and that
    these are only counted once if there is no charger.
    """
    assert_equal(segment.excess_charge(capacity), expected)

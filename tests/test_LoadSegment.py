import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

from pyvrp._pyvrp import LoadSegment

_INT_MAX = np.iinfo(np.int64).max


@pytest.mark.parametrize(
    ("delivery", "pickup", "load"),
    [(1, 2, 3), (0, 0, 0), (_INT_MAX, _INT_MAX, _INT_MAX)],
)
def test_attribute_getters(delivery: int, pickup: int, load: int):
    """
    Tests that the attribute member functions return the passed in values.
    """
    load_segment = LoadSegment(delivery, pickup, load)
    assert_equal(load_segment.delivery(), delivery)
    assert_equal(load_segment.pickup(), pickup)
    assert_equal(load_segment.load(), load)


@pytest.mark.parametrize(
    ("first", "second", "exp_delivery", "exp_pickup", "exp_load"),
    [
        (
            LoadSegment(delivery=5, pickup=8, load=8),
            LoadSegment(delivery=3, pickup=9, load=11),
            5 + 3,  # = sum of delivery
            8 + 9,  # = sum of pickup
            max(8 + 3, 11 + 8),  # = max(load1 + delivery2, load2 + pickup1)
        ),
        (
            LoadSegment(delivery=3, pickup=9, load=11),
            LoadSegment(delivery=5, pickup=8, load=8),
            3 + 5,
            9 + 8,
            max(11 + 5, 8 + 9),
        ),
    ],
)
def test_merge_two(
    first: LoadSegment,
    second: LoadSegment,
    exp_delivery: int,
    exp_pickup: int,
    exp_load: int,
):
    """
    Tests merging two load segments.
    """
    merged = LoadSegment.merge(first, second)
    assert_equal(merged.delivery(), exp_delivery)
    assert_equal(merged.pickup(), exp_pickup)
    assert_equal(merged.load(), exp_load)

    assert_equal(merged.excess_load(0), exp_load)
    assert_equal(merged.excess_load(capacity=exp_load), 0)


def test_excess_load_capacity():
    """
    Tests that excess load is correctly evaluated and merged.
    """
    before = LoadSegment(5, 5, 5, 30)
    after = LoadSegment(2, 2, 2, 5)
    merged = LoadSegment.merge(before, after)

    # There's seven load on this segment, but 30 excess load from some part of
    # the route executed before the last return to the depot, and 5 excess load
    # from part of the route executed after the next return to the depot.
    assert_equal(merged.load(), 7)
    assert_equal(merged.excess_load(7), 35)
    assert_equal(merged.excess_load(0), 42)


@pytest.mark.parametrize(
    ("capacity", "exp_excess"),
    [(10, 20), (5, 20), (0, 25)],
)
def test_finalise(capacity: int, exp_excess: int):
    """
    Tests that excess load is correctly tracked by finalised load segments.
    """
    segment = LoadSegment(5, 5, 5, 20)  # 20 excess load, and 5 segment load
    finalised = segment.finalise(capacity)

    # Finalised segments track cumulative excess load - the rest resets.
    assert_equal(finalised.delivery(), 0)
    assert_equal(finalised.pickup(), 0)
    assert_equal(finalised.load(), 0)
    assert_equal(finalised.excess_load(capacity), exp_excess)


def test_str():
    """
    Tests that the load segment's string representation contains useful
    debugging information.
    """
    segment = LoadSegment(delivery=2, pickup=3, load=5, excess_load=7)
    assert_("delivery=2" in str(segment))
    assert_("pickup=3" in str(segment))
    assert_("load=5" in str(segment))
    assert_("excess_load=7" in str(segment))

import numpy as np
import pytest
from numpy.testing import assert_equal

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

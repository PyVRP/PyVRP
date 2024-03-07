import numpy as np
import pytest
from numpy.testing import assert_equal

from pyvrp._pyvrp import DistanceSegment

_INT_MAX = np.iinfo(np.int32).max


@pytest.mark.parametrize("distance", [-_INT_MAX, 0, 1_00, _INT_MAX])
def test_attribute_getter(distance: int):
    """
    Tests that the distance segment correctly returns the passed-in distance.
    """
    dist_segment = DistanceSegment(0, 1, distance)
    assert_equal(dist_segment.distance(), distance)


@pytest.mark.parametrize(
    ("first", "second", "exp_distance"),
    [
        # 0 -> 1 has dist 1, and since there is no initial distance, total
        # distance on the merged segment is 1.
        (DistanceSegment(0, 0, 0), DistanceSegment(1, 1, 0), 1),
        # First segment has initial distance 2, second has no initial distance.
        # Distance from 2 -> 1 is 1, so total distance is 3.
        (DistanceSegment(0, 2, 2), DistanceSegment(1, 1, 0), 3),
        (DistanceSegment(0, 1, 1), DistanceSegment(2, 2, 0), 2),
        (DistanceSegment(1, 2, 1), DistanceSegment(0, 0, 0), 3),
    ],
)
def test_merge_two(
    first: DistanceSegment, second: DistanceSegment, exp_distance: int
):
    """
    Tests merging two distance segments.
    """
    dist_mat = np.array(
        [
            [0, 1, 2],
            [2, 0, 1],
            [2, 1, 0],
        ]
    )
    merged = DistanceSegment.merge(dist_mat, first, second)
    assert_equal(merged.distance(), exp_distance)

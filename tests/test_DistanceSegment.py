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
    dist_segment = DistanceSegment(distance)
    assert_equal(dist_segment.distance(), distance)


@pytest.mark.parametrize(
    ("frm", "to", "exp_distance"),
    [(0, 0, 0), (2, 1, 1), (2, 0, 2)],
)
def test_merge_two(frm: int, to: int, exp_distance: int):
    """
    Tests merging two distance segments.
    """
    dist_mat = [
        [0, 1, 2],
        [2, 0, 1],
        [2, 1, 0],
    ]

    first = DistanceSegment(0)
    assert_equal(first.distance(), 0)

    second = DistanceSegment(0)
    assert_equal(second.distance(), 0)

    merged = DistanceSegment.merge(dist_mat[frm][to], first, second)
    assert_equal(merged.distance(), exp_distance)
    assert_equal(merged.distance(), dist_mat[frm][to])

import numpy as np
import pytest
from numpy.testing import assert_allclose

from pyvrp._pyvrp import LoadSegment

_INT_MAX = np.iinfo(np.int32).max


@pytest.mark.parametrize(
    ("demand", "supply", "max_load"),
    [(0, 0, 0), (_INT_MAX, _INT_MAX, _INT_MAX)],
)
def test_attribute_getters(demand: int, supply: int, max_load: int):
    """
    Tests that the attribute member functions return the passed in values.
    """
    load_segment = LoadSegment(demand, supply, max_load)
    assert_allclose(load_segment.demand(), demand)
    assert_allclose(load_segment.supply(), supply)
    assert_allclose(load_segment.load(), max_load)


def test_merge_two():
    """
    Tests merging two load segments.
    """
    first = LoadSegment(5, 8, 8)
    second = LoadSegment(3, 9, 11)
    merged = LoadSegment.merge(first, second)

    # Demand and supply are simply the aggregate demand and supply.
    assert_allclose(merged.demand(), 5 + 3)
    assert_allclose(merged.supply(), 8 + 9)

    # Maximum load on the concatenated segment is a bit more involved, as
    # that is computed as max(max_load1 + demand2, max_load2 + supply1).
    assert_allclose(merged.load(), max(8 + 3, 11 + 8))


@pytest.mark.parametrize(
    ("max_load", "capacity", "expected"),
    [
        (0, 0, 0),
        (10, 10, 0),
        (12, 10, 2),
        (25, 0, 25),
        (0, _INT_MAX, 0),
    ],
)
def test_excess_load(max_load: int, capacity: int, expected: int):
    """
    Tests the excess load calculations
    """
    load_segment = LoadSegment(0, 0, max_load)
    assert_allclose(load_segment.excess_load(capacity), expected)

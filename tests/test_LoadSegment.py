import numpy as np
import pytest
from numpy.testing import assert_allclose

from pyvrp._pyvrp import LoadSegment

_INT_MAX = np.iinfo(np.int32).max


@pytest.mark.parametrize(
    ("demand", "supply", "load"),
    [(1, 2, 3), (0, 0, 0), (_INT_MAX, _INT_MAX, _INT_MAX)],
)
def test_attribute_getters(demand: int, supply: int, load: int):
    """
    Tests that the attribute member functions return the passed in values.
    """
    load_segment = LoadSegment(demand, supply, load)
    assert_allclose(load_segment.demand(), demand)
    assert_allclose(load_segment.supply(), supply)
    assert_allclose(load_segment.load(), load)


@pytest.mark.parametrize(
    ("first", "second", "exp_demand", "exp_supply", "exp_load"),
    [
        (
            LoadSegment(demand=5, supply=8, load=8),
            LoadSegment(demand=3, supply=9, load=11),
            5 + 3,  # = sum of demand
            8 + 9,  # = sum of supply
            max(8 + 3, 11 + 8),  # = max(load1 + demand2, load2 + supply1)
        ),
        (
            LoadSegment(demand=3, supply=9, load=11),
            LoadSegment(demand=5, supply=8, load=8),
            3 + 5,
            9 + 8,
            max(11 + 5, 8 + 9),
        ),
    ],
)
def test_merge_two(
    first: LoadSegment,
    second: LoadSegment,
    exp_demand: int,
    exp_supply: int,
    exp_load: int,
):
    """
    Tests merging two load segments.
    """
    merged = LoadSegment.merge(first, second)
    assert_allclose(merged.demand(), exp_demand)
    assert_allclose(merged.supply(), exp_supply)
    assert_allclose(merged.load(), exp_load)

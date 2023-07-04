from typing import List

import pytest
from numpy.testing import assert_equal

from pyvrp.crossover.make_giant_tour import make_giant_tour


@pytest.mark.parametrize(
    "routes, tour",
    [
        ([[5, 4, 3, 2, 1]], [5, 4, 3, 2, 1]),  # one route
        ([[1], [2], [3], [4], [5]], [1, 2, 3, 4, 5]),  # five routes
        ([[1], [], [4, 5, 2, 3]], [1, 4, 5, 2, 3]),  # empty route
    ],
)
def test_make_giant_tour(routes: List[List[int]], tour: List[int]):
    assert_equal(make_giant_tour(routes), tour)

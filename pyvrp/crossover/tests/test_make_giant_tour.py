import pytest
from numpy.testing import assert_equal

from pyvrp.crossover.make_giant_tour import make_giant_tour


@pytest.mark.parametrize(
    "routes, giant_tour",
    [
        ([[5, 4, 3, 2, 1]], [5, 4, 3, 2, 1]),
        ([[1, 2], [3, 4, 5]], [1, 2, 3, 4, 5]),
        ([[], [4, 5], [1, 2, 3]], [4, 5, 1, 2, 3]),
        ([[1, 2, 3], [], [4, 5]], [1, 2, 3, 4, 5]),
    ],
)
def test_make_giant_tour(routes, giant_tour):
    assert_equal(make_giant_tour(routes), giant_tour)

import pytest
from numpy.testing import assert_allclose

from pyvrp import Solution
from pyvrp.diversity import broken_pairs_distance as bpd


def test_bpd_same_solution_is_zero(ok_small):
    """
    Broken pairs distance of a solution with itself should be zero.
    """
    sol1 = Solution(ok_small, [[1, 2, 3, 4]])
    assert_allclose(bpd(sol1, sol1), 0.0)

    sol2 = Solution(ok_small, [[1, 2], [3], [4]])
    assert_allclose(bpd(sol2, sol2), 0.0)


@pytest.mark.parametrize(
    ("routes", "expected"),
    [
        ([[1, 2], [3], [4]], 0.4),  # (2, 3) and (3, 4) are broken
        ([[3], [4, 1, 2]], 0.6),  # (0, 1), (2, 3), (3, 4) are broken
        ([[4, 3, 2, 1]], 0.8),  # reverses reference, so all edges are broken
        # Broken pairs are (3, 4) counted for both 3 and 4 (weight 2) and
        # (4, 0) counted for only 4 (weight 1).
        ([[1, 2, 3]], 0.3),
    ],
)
def test_bpd_calculations_on_examples(ok_small, routes, expected):
    """
    Test BPD calculations of a single-route reference solution of [1, 2, 3, 4]
    w.r.t. alternative routes for an instance with five locations (1 depot, and
    4 clients).
    """
    reference = Solution(ok_small, [[1, 2, 3, 4]])
    alternative = Solution(ok_small, routes)

    # Test that BPD is as expected, and that it is symmetric.
    assert_allclose(bpd(reference, alternative), expected)
    assert_allclose(bpd(alternative, reference), expected)

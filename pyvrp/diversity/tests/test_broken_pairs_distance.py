from numpy.testing import assert_allclose

from pyvrp import Solution
from pyvrp.diversity import broken_pairs_distance as bpd


def test_broken_pairs_distance(ok_small):
    """
    Test broken pairs distance (BPD) calculations on a number of solutions to
    a small instance.
    """
    sol1 = Solution(ok_small, [[1, 2, 3, 4]])
    sol2 = Solution(ok_small, [[1, 2], [3], [4]])
    sol3 = Solution(ok_small, [[3], [4, 1, 2]])
    sol4 = Solution(ok_small, [[4, 3, 2, 1]])
    sol5 = Solution(ok_small, [[1, 2, 3]])

    # BPD of a solution and itself should be zero.
    for sol in [sol1, sol2, sol3, sol4]:
        assert_allclose(bpd(sol, sol), 0.0)

    # BPD of sol1 and sol2. The two broken pairs are (2, 3) and (3, 4).
    assert_allclose(bpd(sol1, sol2), 0.5)
    assert_allclose(bpd(sol2, sol1), 0.5)

    # BPD of sol1 and sol3. The three broken pairs are (0, 1), (2, 3), (3, 4).
    assert_allclose(bpd(sol1, sol3), 0.75)
    assert_allclose(bpd(sol3, sol1), 0.75)

    # BPD of sol1 and sol4. sol4 reverses sol1, so all pairs are broken.
    assert_allclose(bpd(sol1, sol4), 1.0)
    assert_allclose(bpd(sol4, sol1), 1.0)

    # BPD of sol2 and sol3. The broken pair is (0, 1).
    assert_allclose(bpd(sol2, sol3), 0.25)
    assert_allclose(bpd(sol3, sol2), 0.25)

    # BPD of sol1 and sol5. Broken pairs are (3, 4) counted for both 3 and 4
    # (weight 2) and (4, 0) counted for only 4 (weight 1), so 3/8.
    assert_allclose(bpd(sol1, sol5), 0.375)
    assert_allclose(bpd(sol5, sol1), 0.375)

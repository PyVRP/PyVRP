from numpy.testing import assert_equal

import pyvrp
from pyvrp.search._search import Solution


def test_load_unload(ok_small):
    """
    Tests that loading and then unloading an unchanged solution returns the
    same original solution.
    """
    pyvrp_sol = pyvrp.Solution(ok_small, [[1, 2], [3, 4]])

    search_sol = Solution(ok_small)
    search_sol.load(ok_small, pyvrp_sol)
    assert_equal(search_sol.unload(ok_small), pyvrp_sol)


# TODO test routes/nodes

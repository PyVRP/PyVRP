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


def test_nodes_routes_access(ok_small):
    """
    Tests nodes and routes access.
    """
    pyvrp_sol = pyvrp.Solution(ok_small, [[1, 2], [3, 4]])
    search_sol = Solution(ok_small)
    search_sol.load(ok_small, pyvrp_sol)

    # There should be #locations nodes, and #vehicles routes.
    assert_equal(len(search_sol.nodes), ok_small.num_locations)
    assert_equal(len(search_sol.routes), ok_small.num_vehicles)

    for client in [1, 2]:  # [1, 2] are in the first route
        assert_equal(search_sol.nodes[client].client, client)
        assert_equal(search_sol.nodes[client].route, search_sol.routes[0])

    for client in [3, 4]:  # [3, 4] are in the second route
        assert_equal(search_sol.nodes[client].client, client)
        assert_equal(search_sol.nodes[client].route, search_sol.routes[1])

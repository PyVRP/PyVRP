from numpy.testing import assert_, assert_equal

import pyvrp
from pyvrp import CostEvaluator
from pyvrp.search import compute_neighbours
from pyvrp.search._search import SearchSpace, Solution


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


def test_insert_required(ok_small):
    """
    Tests that inserting clients can fail if inserting is too expensive and
    the insert is not required.
    """
    data = ok_small
    search_space = SearchSpace(data, compute_neighbours(data))
    cost_eval = CostEvaluator([0], 0, 0)

    # Start with an empty solution and try to insert the first client without
    # requiring an insert. Inserting should fail: it's not worth it, since the
    # client has no prize. However, inserting should succeed when required.
    sol = Solution(data)
    assert_(not sol.insert(sol.nodes[1], search_space, data, cost_eval, False))
    assert_(sol.insert(sol.nodes[1], search_space, data, cost_eval, True))

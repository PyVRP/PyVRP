from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator, DynamicBitset, Solution
from pyvrp.repair import greedy_repair
from pyvrp.tests.helpers import read


def test_empty_unplanned_is_a_no_op():
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    unplanned = DynamicBitset(data.num_clients + 1)
    assert_equal(unplanned.count(), 0)

    # When unplanned is empty, there is nothing for greedy repair to do, so it
    # should return the exact same solution it received.
    sol = Solution(data, [[3, 2], [1, 4]])
    assert_equal(greedy_repair(sol, unplanned, data, cost_eval), sol)

    # This is also true when the solution is not complete: greedy repair only
    # reinserts what's in unplanned.
    sol = Solution(data, [[2, 3, 4]])
    assert_(not sol.is_complete())
    assert_equal(greedy_repair(sol, unplanned, data, cost_eval), sol)


def test_after_depot():
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    # We want to insert client 4 into the following single-route solution. It
    # is optimal to do so directly after the depot, just before client 3.
    sol = Solution(data, [[3, 2, 1]])
    unplanned = DynamicBitset(data.num_clients + 1)
    unplanned[4] = True

    # The greedy repair operator inserts into *existing* routes; it does not
    # create new ones.
    repaired = greedy_repair(sol, unplanned, data, cost_eval)
    assert_equal(sol.num_routes(), repaired.num_routes())

    # Let's check if the repaired solution indeed visits client 4 first.
    route = repaired.get_routes()[0]
    assert_equal(route.visits(), [4, 3, 2, 1])

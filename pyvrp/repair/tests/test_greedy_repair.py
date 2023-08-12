from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator, DynamicBitset, RandomNumberGenerator, Solution
from pyvrp.repair import greedy_repair
from pyvrp.tests.helpers import read


def test_empty_unplanned_is_a_no_op():
    data = read("data/OkSmall.txt")
    rng = RandomNumberGenerator(seed=42)

    sol = Solution.make_random(data, rng)
    unplanned = DynamicBitset(data.num_clients + 1)
    cost_eval = CostEvaluator(1, 1)

    # When unplanned is empty, there is nothing for greedy repair to do, so it
    # should return the exact same solution it received.
    assert_equal(unplanned.count(), 0)
    assert_equal(greedy_repair(sol, unplanned, data, cost_eval), sol)

    # This is also true when the solution is not complete: greedy repair only
    # reinserts what's in unplanned.
    sol = Solution(data, [[2, 3, 4]])
    assert_(not sol.is_complete())
    assert_equal(greedy_repair(sol, unplanned, data, cost_eval), sol)

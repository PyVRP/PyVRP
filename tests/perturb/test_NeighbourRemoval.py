import pytest
from numpy.testing import assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.perturb import DestroyRepair, NeighbourRemoval
from pyvrp.search.neighbourhood import compute_neighbours


@pytest.mark.parametrize("num_removals", range(4))
def test_remove_correct_number_of_clients(ok_small, num_removals: int):
    """
    Tests that calling neighbour removal removes the correct number of clients.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)

    dr = DestroyRepair(ok_small)
    dr.add_destroy_operator(
        NeighbourRemoval(ok_small, num_removals=num_removals)
    )
    destroyed = dr(sol, cost_eval, neighbours, rng)

    assert_equal(destroyed.num_clients() + num_removals, sol.num_clients())


def test_no_neighbours_is_no_op(ok_small):
    """
    Tests that calling neighbour removal with no neighbours is a no-op.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [], [], [], []]
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)

    dr = DestroyRepair(ok_small)
    dr.add_destroy_operator(NeighbourRemoval(ok_small, num_removals=4))
    destroyed = dr(sol, cost_eval, neighbours, rng)

    assert_equal(destroyed, sol)


def test_removes_at_most_number_of_neighbours(ok_small):
    """
    Tests that calling neighbour removal does not remove more clients than
    there are neighbours.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [2], [3], [4], [1]]
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)

    dr = DestroyRepair(ok_small)
    dr.add_destroy_operator(NeighbourRemoval(ok_small, num_removals=4))
    destroyed = dr(sol, cost_eval, neighbours, rng)

    # There is just one neighbour for each client, so only one is removed.
    assert_equal(destroyed.num_clients() + 1, sol.num_clients())

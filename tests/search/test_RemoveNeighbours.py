from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import LocalSearch, RemoveNeighbours
from pyvrp.search.neighbourhood import compute_neighbours


def test_neighbour_removal_no_op_empty_solution(ok_small):
    """
    Tests that calling RemoveNeighbours on an empty solution is a no-op,
    since there are no clients to remove.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(RemoveNeighbours(ok_small))

    sol = Solution(ok_small, [])
    cost_eval = CostEvaluator([1], 1, 0)
    assert_equal(ls.perturb(sol, cost_eval), sol)


def test_returns_same_solution_with_empty_neighbourhood(ok_small):
    """
    Tests that calling RemoveNeighbours with an empty neighbourhood is a no-op:
    since the neighbourhood removal operator uses neighbours, it cannot do
    anything with an empty neighbourhood.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [], [], [], []]
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(RemoveNeighbours(ok_small))

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 0)
    assert_equal(ls.perturb(sol, cost_eval), sol)


def test_removes_clients(ok_small):
    """
    Tests that calling RemoveNeighbours removes clients.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(RemoveNeighbours(ok_small))

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([20], 6, 0)

    destroyed = ls.perturb(sol, cost_eval)
    assert_(destroyed.num_clients() < sol.num_clients())

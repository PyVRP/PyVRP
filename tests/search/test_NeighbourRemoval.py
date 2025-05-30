import pytest
from numpy.testing import assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import LocalSearch, NeighbourRemoval
from pyvrp.search.neighbourhood import compute_neighbours


@pytest.mark.parametrize("num_destroy", range(4))
def test_remove_correct_number_of_clients(ok_small, num_destroy: int):
    """
    Tests that calling neighbour removal removes the correct number of clients.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_destroy_operator(NeighbourRemoval(num_destroy))

    sol = Solution.make_random(ok_small, rng)
    destroyed = ls.destroy(sol, cost_evaluator)

    assert_equal(destroyed.num_clients() + num_destroy, sol.num_clients())


def test_local_search_returns_same_solution_with_empty_neighbourhood(ok_small):
    """
    Tests that calling the local search destroy step combined with an empty
    neighbourhood is a no-op: since the neighbourhood removals use the
    neighours, they cannot do anything with an empty neighbourhood.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [], [], [], []]
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_destroy_operator(NeighbourRemoval(4))

    destroyed = ls.destroy(sol, cost_eval)

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
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_destroy_operator(NeighbourRemoval(4))

    destroyed = ls.destroy(sol, cost_eval)

    # There is just one neighbour for each client, so only one is removed.
    assert_equal(destroyed.num_clients() + 1, sol.num_clients())


def test_selects_random_client(ok_small):
    """
    Tests that calling neighbour removal selects a random client every time,
    of which the neighbours are used to remove clients.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [2, 3, 4], [1, 3, 4], [1, 2, 4], [1, 2, 3]]
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)

    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_destroy_operator(NeighbourRemoval(3))

    # We run the destroy operator multiple times and store the resulting
    # solutions to deal with the randomness in the selection of the client.
    solutions = set(ls.destroy(sol, cost_eval) for _ in range(20))

    # The neighborhood structure removes all clients except the selected one.
    # Therefore, we expect to find all singleton solutions, each containing
    # exactly one client.
    singletons = {Solution(ok_small, [[idx]]) for idx in range(1, 5)}
    assert_equal(solutions, singletons)

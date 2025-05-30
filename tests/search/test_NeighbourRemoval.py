import pytest
from numpy.testing import assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import LocalSearch, NeighbourRemoval
from pyvrp.search.neighbourhood import compute_neighbours


def test_neighbour_removal_no_op_empty_solution(ok_small):
    """
    Tests that calling neighbour removal on an empty solution is a no-op,
    since there are no clients to remove.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    cost_eval = CostEvaluator([1], 1, 0)

    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_destroy_operator(NeighbourRemoval(ok_small, 4))

    sol = Solution(ok_small, [])
    assert_equal(ls.destroy(sol, cost_eval), sol)


def test_local_search_returns_same_solution_with_empty_neighbourhood(ok_small):
    """
    Tests that calling the local search destroy step combined with an empty
    neighbourhood is a no-op: since the neighbourhood removal operator uses the
    neighours, it cannot do anything with an empty neighbourhood.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [], [], [], []]
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)

    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_destroy_operator(NeighbourRemoval(ok_small, 4))

    assert_equal(ls.destroy(sol, cost_eval), sol)


def test_neighbour_removal_removes_at_most_number_of_neighbours(ok_small):
    """
    Tests that calling neighbour removal does not remove more clients than
    there are neighbours.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [2], [3], [4], [1]]
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)

    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_destroy_operator(NeighbourRemoval(ok_small, 4))

    # There is just one neighbour per client, so only one neighbour is removed.
    destroyed = ls.destroy(sol, cost_eval)
    assert_equal(destroyed.num_clients() + 1, sol.num_clients())


@pytest.mark.parametrize("num_destroy", range(4))
def test_operator_removes_correct_number_of_clients(
    ok_small, num_destroy: int
):
    """
    Tests that calling neighbour removal removes the correct number of clients.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_destroy_operator(NeighbourRemoval(ok_small, num_destroy))

    sol = Solution.make_random(ok_small, rng)
    destroyed = ls.destroy(sol, cost_evaluator)

    assert_equal(destroyed.num_clients() + num_destroy, sol.num_clients())


def test_neighbour_removal_selects_random_client(ok_small):
    """
    Tests that calling neighbour removal selects a random client every time,
    of which the neighbours are used to remove clients.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [2, 3, 4], [1, 3, 4], [1, 2, 4], [1, 2, 3]]
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)

    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_destroy_operator(NeighbourRemoval(ok_small, 3))

    # We run the destroy operator multiple times and store the resulting
    # solutions to deal with the randomness in the selection of the client.
    solutions = set(ls.destroy(sol, cost_eval) for _ in range(20))

    # Every client has all other clients as neighbours, so the operator will
    # remove all clients except the one that was selected. We therefore expect
    # to find all singleton solutions, each having exactly one client.
    singletons = {Solution(ok_small, [[idx]]) for idx in range(1, 5)}
    assert_equal(solutions, singletons)
    assert_equal(len(solutions), 4)

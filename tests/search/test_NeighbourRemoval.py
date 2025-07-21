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
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(NeighbourRemoval(ok_small, 4))

    sol = Solution(ok_small, [])
    cost_eval = CostEvaluator([1], 1, 0)
    assert_equal(ls.perturb(sol, cost_eval), sol)


def test_returns_same_solution_with_empty_neighbourhood(ok_small):
    """
    Tests that calling the neighbour removal combined with an empty
    neighbourhood is a no-op: since the neighbourhood removal operator uses
    neighbours, it cannot do anything with an empty neighbourhood.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [], [], [], []]
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(NeighbourRemoval(ok_small, 4))

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 0)
    assert_equal(ls.perturb(sol, cost_eval), sol)


def test_neighbour_removal_removes_at_most_number_of_neighbours(ok_small):
    """
    Tests that calling neighbour removal does not remove more clients than
    there are neighbours.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [2], [3], [4], [1]]
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(NeighbourRemoval(ok_small, 4))

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 0)

    # There is just one neighbour per client, so we only remove one instead
    # of four.
    destroyed = ls.perturb(sol, cost_eval)
    assert_equal(destroyed.num_clients() + 1, sol.num_clients())


@pytest.mark.parametrize("num_perturb", range(4))
def test_remove_correct_number_of_clients(ok_small, num_perturb: int):
    """
    Tests that calling neighbour removal removes the correct number of clients.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(NeighbourRemoval(ok_small, num_perturb))

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([20], 6, 0)

    destroyed = ls.perturb(sol, cost_eval)
    assert_equal(destroyed.num_clients() + num_perturb, sol.num_clients())


def test_neighbour_removal_selects_random_client(ok_small):
    """
    Tests that each call to neighbour removal randomly selects a client, whose
    neighbours are then used to remove other clients.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [2, 3, 4], [1, 3, 4], [1, 2, 4], [1, 2, 3]]
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(NeighbourRemoval(ok_small, 3))

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 0)

    # We run the destroy operator multiple times and store the resulting
    # solutions to deal with the randomness in the selection of the client.
    solutions = set(ls.perturb(sol, cost_eval) for _ in range(20))

    # Every client has all other clients as neighbours, so the operator will
    # remove all clients except the one that was selected. We thus expect to
    # find all singleton solutions in the end, each having exactly one client.
    singletons = {Solution(ok_small, [[idx]]) for idx in range(1, 5)}
    assert_equal(solutions, singletons)
    assert_equal(len(solutions), 4)

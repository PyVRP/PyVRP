import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import LocalSearch, OptionalInsert
from pyvrp.search.neighbourhood import compute_neighbours


def test_no_op_empty_solution(ok_small):
    """
    Tests that calling optional insert on an empty solution without optional
    clients is a no-op, since there are no clients to insert.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(OptionalInsert(ok_small, 4))

    sol = Solution(ok_small, [])  # type: ignore
    cost_eval = CostEvaluator([1], 1, 0)
    assert_equal(ls.perturb(sol, cost_eval), sol)


def test_insert_in_empty_routes(ok_small_prizes):
    """
    Tests that optional insert also inserts clients into empty routes.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [], [], [], []]
    ls = LocalSearch(ok_small_prizes, rng, neighbours)
    ls.add_perturbation_operator(OptionalInsert(ok_small_prizes, 1))

    sol = Solution(ok_small_prizes, [])  # type: ignore
    cost_eval = CostEvaluator([20], 6, 0)

    perturbed = ls.perturb(sol, cost_eval)
    assert_equal(perturbed.num_clients(), 1)


@pytest.mark.parametrize("num_perturb", range(4))
def test_insert_correct_number_of_clients(ok_small_prizes, num_perturb: int):
    """
    Tests that calling optional insert inserts the correct number of clients.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small_prizes)
    ls = LocalSearch(ok_small_prizes, rng, neighbours)
    ls.add_perturbation_operator(OptionalInsert(ok_small_prizes, num_perturb))

    sol = Solution(ok_small_prizes, [])  # type: ignore
    cost_eval = CostEvaluator([20], 6, 0)

    perturbed = ls.perturb(sol, cost_eval)
    assert_equal(perturbed.num_clients(), sol.num_clients() + num_perturb)


def test_supports(ok_small, ok_small_prizes):
    """
    Tests that OptionalInsert does not support instances without optional
    clients.
    """
    assert_(not OptionalInsert.supports(ok_small))  # no optional clients
    assert_(OptionalInsert.supports(ok_small_prizes))  # has optional clients

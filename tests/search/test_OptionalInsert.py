from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import LocalSearch, OptionalInsert
from pyvrp.search.neighbourhood import compute_neighbours


def test_no_op_empty_solution(ok_small):
    """
    Tests that calling OptionalInsert on an empty solution without optional
    clients is a no-op, since there are no clients to insert.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(OptionalInsert(ok_small))

    sol = Solution(ok_small, [])  # type: ignore
    cost_eval = CostEvaluator([1], 1, 0)
    assert_equal(ls.perturb(sol, cost_eval), sol)


def test_inserts_optional_clients(ok_small_prizes):
    """
    Tests that OptionalInsert only inserts optional clients and does not
    insert required clients.
    """
    rng = RandomNumberGenerator(seed=0)
    neighbours = compute_neighbours(ok_small_prizes)
    ls = LocalSearch(ok_small_prizes, rng, neighbours)
    ls.add_perturbation_operator(OptionalInsert(ok_small_prizes))

    sol = Solution(ok_small_prizes, [])  # type: ignore
    cost_eval = CostEvaluator([20], 6, 0)

    perturbed = ls.perturb(sol, cost_eval)
    assert_equal(perturbed.num_clients(), 3)

    visits = {visit for route in perturbed.routes() for visit in route}
    assert_(1 not in visits)  # client 1 is required, so not inserted


def test_inserts_in_empty_routes(ok_small_prizes):
    """
    Tests that OptionalInsert also inserts clients into empty routes.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = [[], [], [], [], []]  # no neighbors
    ls = LocalSearch(ok_small_prizes, rng, neighbours)
    ls.add_perturbation_operator(OptionalInsert(ok_small_prizes))

    sol = Solution(ok_small_prizes, [])  # type: ignore
    cost_eval = CostEvaluator([20], 6, 0)

    # This should still insert clients even when there are no neighboring
    # clients because empty routes will be used.
    perturbed = ls.perturb(sol, cost_eval)
    assert_(perturbed.num_clients() > 0)


def test_supports(ok_small, ok_small_prizes):
    """
    Tests that OptionalInsert does not support instances without optional
    clients.
    """
    assert_(not OptionalInsert.supports(ok_small))  # no optional clients
    assert_(OptionalInsert.supports(ok_small_prizes))  # has optional clients

from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution, read_solution
from pyvrp.perturb import DestroyRepair, GreedyRepair, NeighbourRemoval
from pyvrp.search.neighbourhood import compute_neighbours


def test_no_op_results_in_same_solution(ok_small):
    """
    Tests that calling destroy and repair with no operators is a no-op,
    and returns the same solution as the one given as input.
    """
    dr = DestroyRepair(ok_small)
    rng = RandomNumberGenerator(seed=42)
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution.make_random(ok_small, rng)
    neighbours = compute_neighbours(ok_small)

    assert_equal(dr(sol, cost_eval, neighbours, rng), sol)


def test_destroy_and_repair_improves_random_solution(ok_small):
    """
    Tests that calling destroy and repair with a random solution results in an
    improved solution.
    """

    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    cost_eval = CostEvaluator([1], 1, 1)
    sol = Solution.make_random(ok_small, rng)

    dr = DestroyRepair(ok_small)
    dr.add_destroy_operator(NeighbourRemoval(ok_small, num_removals=3))
    dr.add_repair_operator(GreedyRepair(ok_small))

    perturbed = dr(sol, cost_eval, neighbours, rng)

    cost_before = cost_eval.penalised_cost(sol)
    cost_after = cost_eval.penalised_cost(perturbed)
    assert_(cost_after < cost_before)


def test_destroy_and_repair_worsens_optimal_solution(rc208):
    """
    Tests that calling destroy and repair can worsen an optimal solution.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(rc208)
    cost_eval = CostEvaluator([1], 1, 1)
    sol = read_solution("tests/data/RC208.sol", rc208)

    dr = DestroyRepair(rc208)
    dr.add_destroy_operator(NeighbourRemoval(rc208, num_removals=20))
    dr.add_repair_operator(GreedyRepair(rc208))

    perturbed = dr(sol, cost_eval, neighbours, rng)

    # If destroy and repair is called with a high `num_removals`, then it is
    # expected that the perturbed solution is worse than the original one.
    cost_before = cost_eval.penalised_cost(sol)
    cost_after = cost_eval.penalised_cost(perturbed)
    assert_(cost_after > cost_before)

from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import (
    IteratedLocalSearch,
    IteratedLocalSearchParams,
    PenaltyManager,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.accept import MovingBestAverageThreshold
from pyvrp.search import (
    Exchange10,
    LocalSearch,
    NeighbourRemoval,
    compute_neighbours,
)
from pyvrp.stop import MaxIterations
from tests.helpers import read_solution


@mark.parametrize(
    "num_iters_no_improvement",
    [(-1)],  # num_iters_no_improvement < 0
)
def test_params_constructor_raises_when_arguments_invalid(
    num_iters_no_improvement: int,
):
    """
    Tests that invalid configurations are not accepted.
    """
    with assert_raises(ValueError):
        IteratedLocalSearchParams(num_iters_no_improvement)


@mark.parametrize("num_iters_no_improvement", range(5))
def test_params_constructor_does_not_raise_when_arguments_valid(
    num_iters_no_improvement: int,
):
    """
    Tests valid boundary cases.
    """
    params = IteratedLocalSearchParams(num_iters_no_improvement)
    assert_equal(params.num_iters_no_improvement, num_iters_no_improvement)


def test_best_solution_improves_with_more_iterations(rc208):
    """
    Tests that additional iterations result in better solutions. This is a
    smoke test that checks at least something's improving during the ILS's
    execution.
    """
    rng = RandomNumberGenerator(seed=42)
    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208))
    ls.add_node_operator(Exchange10(rc208))
    accept = MovingBestAverageThreshold(0, 100)
    init = Solution.make_random(rc208, rng)
    algo = IteratedLocalSearch(rc208, pm, rng, ls, accept, init)

    initial_best = algo.run(MaxIterations(0)).best
    new_best = algo.run(MaxIterations(25)).best

    cost_evaluator = pm.cost_evaluator()
    new_best_cost = cost_evaluator.penalised_cost(new_best)
    initial_best_cost = cost_evaluator.penalised_cost(initial_best)
    assert_(new_best_cost < initial_best_cost)
    assert_(new_best.is_feasible())  # best must be feasible


def test_best_initial_solution(rc208):
    """
    Tests that ILS uses the initial solution to initialise the best found
    solution.
    """
    rng = RandomNumberGenerator(seed=42)
    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208))
    accept = MovingBestAverageThreshold(0, 100)
    bks = read_solution("data/RC208.sol", rc208)
    algo = IteratedLocalSearch(rc208, pm, rng, ls, accept, bks)

    result = algo.run(MaxIterations(0))

    # The best known solution is the initial solution. Since we don't run any
    # iterations, this should be returned as best solution.
    assert_equal(result.best, bks)


def test_restarts_after_no_improvement(rc208):
    """
    Tests that ILS restarts the search after a number of iterations without
    improvement.
    """
    rng = RandomNumberGenerator(seed=42)
    pm = PenaltyManager(initial_penalties=([1000], 1000, 1000))

    ls = LocalSearch(rc208, rng, compute_neighbours(rc208))
    ls.add_perturbation_operator(NeighbourRemoval(rc208, 20))
    ls.add_node_operator(Exchange10(rc208))

    accept = MovingBestAverageThreshold(1, 1)  # always accept
    bks = read_solution("data/RC208.sol", rc208)
    ils_params = IteratedLocalSearchParams(num_iters_no_improvement=3)
    algo = IteratedLocalSearch(rc208, pm, rng, ls, accept, bks, ils_params)

    result = algo.run(MaxIterations(3))
    data = result.stats.data

    # The initial solution should match the cost of the BKS.
    bks_cost = pm.cost_evaluator().penalised_cost(bks)
    assert_equal(data[0].current_cost, bks_cost)

    # The candidate found in the first iteration is worse than the BKS, but
    # still accepted because of the acceptance criterion.
    assert_(data[0].candidate_cost > bks_cost)
    assert_equal(data[1].current_cost, data[0].candidate_cost)

    # The candidate in the second iteration is also worse and accepted.
    # However, ILS restarts the search in the third iteration as there were
    # not enough improving iterations, so the current cost is set to the BKS.
    assert_(data[1].candidate_cost > bks_cost)
    assert_equal(data[2].current_cost, bks_cost)


def test_ils_result_has_correct_stats(ok_small):
    """
    Tests that ILS correctly collects search statistics.
    """
    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    rng = RandomNumberGenerator(42)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small))
    accept = MovingBestAverageThreshold(0, 100)
    init = Solution.make_random(ok_small, rng)
    ils = IteratedLocalSearch(ok_small, pm, rng, ls, accept, init)

    result = ils.run(MaxIterations(10), collect_stats=True)
    assert_equal(result.best, init)
    assert_equal(result.num_iterations, 10)
    assert_equal(result.stats.num_iterations, 10)

    datum = result.stats.data[0]
    assert_equal(datum.current_cost, pm.cost_evaluator().penalised_cost(init))
    assert_equal(datum.current_feas, init.is_feasible())

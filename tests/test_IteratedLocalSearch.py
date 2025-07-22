from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import (
    IteratedLocalSearch,
    IteratedLocalSearchParams,
    PenaltyManager,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.search import (
    Exchange10,
    LocalSearch,
    NeighbourRemoval,
    compute_neighbours,
)
from pyvrp.stop import FirstFeasible, MaxIterations
from tests.helpers import read_solution


@mark.parametrize(
    "num_iters_no_improvement, initial_accept_weight, history_length",
    [
        (-1, 1, 1),  # num_iters_no_improvement < 0
        (0, -1, 1),  # initial_accept_weight < 0
        (0, 2, 1),  # initial_accept_weight > 1
        (0, 1, 0),  # history_length < 1
    ],
)
def test_params_constructor_raises_when_arguments_invalid(
    num_iters_no_improvement: int,
    initial_accept_weight: float,
    history_length: int,
):
    """
    Tests that invalid configurations are not accepted.
    """
    with assert_raises(ValueError):
        IteratedLocalSearchParams(
            num_iters_no_improvement=num_iters_no_improvement,
            initial_accept_weight=initial_accept_weight,
            history_length=history_length,
        )


@mark.parametrize(
    "num_iters_no_improvement, initial_accept_weight, history_length",
    [
        (0, 1, 1),  # num_iters_no_improvement == 0
        (0, 0, 1),  # initial_accept_weight == 0
        (0, 1, 1),  # initial_accept_weight == 1
        (0, 1, 1),  # history_length == 1
    ],
)
def test_params_constructor_does_not_raise_when_arguments_valid(
    num_iters_no_improvement: int,
    initial_accept_weight: float,
    history_length: int,
):
    """
    Tests valid boundary cases.
    """
    IteratedLocalSearchParams(
        num_iters_no_improvement=num_iters_no_improvement,
        initial_accept_weight=initial_accept_weight,
        history_length=history_length,
    )


def test_best_solution_improves_with_more_iterations(rc208):
    """
    Tests that additional iterations result in better solutions. This is a
    smoke test that checks at least something's improving during the ILS's
    execution.
    """
    rng = RandomNumberGenerator(seed=42)
    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208))
    ls.add_perturbation_operator(NeighbourRemoval(rc208))
    ls.add_node_operator(Exchange10(rc208))
    init = Solution.make_random(rc208, rng)
    algo = IteratedLocalSearch(rc208, pm, rng, ls, init)

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
    bks = read_solution("data/RC208.sol", rc208)
    algo = IteratedLocalSearch(rc208, pm, rng, ls, bks)

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
    pm = PenaltyManager(initial_penalties=([20], 6, 6))

    ls = LocalSearch(
        rc208, rng, compute_neighbours(rc208), num_perturbations=50
    )
    ls.add_perturbation_operator(NeighbourRemoval(rc208))
    ls.add_node_operator(Exchange10(rc208))

    bks = read_solution("data/RC208.sol", rc208)
    ils_params = IteratedLocalSearchParams(
        num_iters_no_improvement=3,
        history_length=1,  # accept all candidate solutions
    )
    algo = IteratedLocalSearch(rc208, pm, rng, ls, bks, ils_params)

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
    init = Solution.make_random(ok_small, rng)
    ils = IteratedLocalSearch(ok_small, pm, rng, ls, init)

    result = ils.run(MaxIterations(10), collect_stats=True)
    assert_equal(result.best, init)
    assert_equal(result.num_iterations, 10)
    assert_equal(result.stats.num_iterations, 10)

    datum = result.stats.data[0]
    assert_equal(datum.current_cost, pm.cost_evaluator().penalised_cost(init))
    assert_equal(datum.current_feas, init.is_feasible())


def test_ils_accepts_below_threshold(ok_small):
    """
    Tests that ILS accepts candidates that are below the threshold.
    """

    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    rng = RandomNumberGenerator(42)
    init = Solution.make_random(ok_small, rng)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small))
    params = IteratedLocalSearchParams(initial_accept_weight=0.5)
    ils = IteratedLocalSearch(ok_small, pm, rng, ls, init, params)
    candidate = Solution(ok_small, [[1, 2], [3, 4]])  # cost 9725

    # Threshold is 0.5 * 10000 + 0.5 * 0.5 * (10000 + 9725) / 2 = 9931.25,
    # so the candidate solution is accepted.
    ils._history.extend([10000])  # noqa
    assert_(ils._accept(candidate, candidate, FirstFeasible()))  # noqa


def test_ils_rejects_above_threshold(ok_small):
    """
    Tests that ILS rejects candidates that are above the threshold.
    """
    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    rng = RandomNumberGenerator(42)
    init = Solution.make_random(ok_small, rng)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small))
    params = IteratedLocalSearchParams(initial_accept_weight=0.5)
    ils = IteratedLocalSearch(ok_small, pm, rng, ls, init, params)
    candidate = Solution(ok_small, [[1, 2], [3, 4]])  # cost 9725

    # Threshold is 0.5 * 9500 + 0.5 * (9500 + 9725) / 2 = 9556.25,
    # so the candidate solution is rejected.
    ils._history.extend([0, 1])  # noqa
    assert_(not ils._accept(candidate, candidate, FirstFeasible()))  # noqa


def test_ils_accepts_at_threshold(ok_small):
    """
    Tests that ILS rejects candidates that are above the threshold.
    """
    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    rng = RandomNumberGenerator(42)
    init = Solution.make_random(ok_small, rng)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small))
    params = IteratedLocalSearchParams(initial_accept_weight=0.5)
    ils = IteratedLocalSearch(ok_small, pm, rng, ls, init, params)
    candidate = Solution(ok_small, [[1, 2], [3, 4]])  # cost 9725

    # Threshold is precisely equal to the candidate cost, so accept.
    ils._history.extend([9725])  # noqa
    assert_(ils._accept(candidate, candidate, FirstFeasible()))  # noqa


def test_ils_rejects_due_to_stopping_criterion(ok_small):
    """
    Tests that ILS correctly rejects a solution because the stopping
    criterion has been met.
    """
    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    rng = RandomNumberGenerator(42)
    init = Solution.make_random(ok_small, rng)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small))
    params = IteratedLocalSearchParams(initial_accept_weight=1)
    ils = IteratedLocalSearch(ok_small, pm, rng, ls, init, params)
    candidate = Solution(ok_small, [[1, 2], [3, 4]])  # cost 9725

    ils._history.extend([9725 * 2, 0])  # noqa

    # Threshold is 9725 (weight=1 with average of 9725). The first feasible
    # criterion does not have a meaningful fraction remaining, so the candidate
    # solution is accepted.
    assert_(ils._accept(candidate, candidate, FirstFeasible()))  # noqa

    # The max iterations criterion's fraction remaining is now zero, so the
    # threshold is equal to the recent best (0) and the candidate is rejected.
    assert_(not ils._accept(candidate, candidate, MaxIterations(0)))  # noqa


def test_ils_accepts_when_best_is_infeasible(ok_small):
    """
    Tests that ILS accepts a candidate solution when the best solution is
    infeasible.
    """
    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    rng = RandomNumberGenerator(42)
    init = Solution.make_random(ok_small, rng)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small))
    params = IteratedLocalSearchParams(initial_accept_weight=1)
    ils = IteratedLocalSearch(ok_small, pm, rng, ls, init, params)

    candidate = Solution(ok_small, [[1, 2], [3, 4]])  # cost 9725
    ils._history.extend([9500])  # noqa

    # Candidate solution is accepted despite being worse than the threshold,
    # because the best solution is infeasible.
    infeas = Solution(ok_small, [[1, 2, 3], [4]])
    assert_(not infeas.is_feasible())
    assert_(ils._accept(candidate, infeas, FirstFeasible()))  # noqa

    # The best solution is now infeasible, so the candidate is rejected.
    feas = Solution(ok_small, [[1, 2], [3], [4]])
    assert_(feas.is_feasible())
    assert_(not ils._accept(candidate, feas, MaxIterations(0)))  # noqa

# This file is part of the PyVRP project (https://github.com/PyVRP/PyVRP),
# licensed under the terms of the MIT license.

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
    PerturbationManager,
    PerturbationParams,
    compute_neighbours,
)
from pyvrp.stop import MaxIterations
from tests.helpers import read_solution


@mark.parametrize(
    "num_iters_no_improvement, history_length",
    [
        (-1, 1),  # num_iters_no_improvement < 0
        (0, 0),  # history_length < 1
    ],
)
def test_params_constructor_raises_when_arguments_invalid(
    num_iters_no_improvement: int,
    history_length: int,
):
    """
    Tests that invalid configurations are not accepted.
    """
    with assert_raises(ValueError):
        IteratedLocalSearchParams(
            num_iters_no_improvement=num_iters_no_improvement,
            history_length=history_length,
        )


@mark.parametrize(
    "num_iters_no_improvement, history_length",
    [
        (0, 1),  # num_iters_no_improvement == 0
        (0, 1),  # history_length == 1
    ],
)
def test_params_constructor_does_not_raise_when_arguments_valid(
    num_iters_no_improvement: int,
    history_length: int,
):
    """
    Tests valid boundary cases.
    """
    IteratedLocalSearchParams(
        num_iters_no_improvement,
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


def test_ils_result_has_correct_stats(ok_small):
    """
    Tests that ILS correctly collects search statistics.
    """
    params = PerturbationParams(0, 0)  # disable perturbation
    perturbation = PerturbationManager(params)

    pm = PenaltyManager(initial_penalties=([20], 6, 6))
    rng = RandomNumberGenerator(42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours, perturbation)
    init = Solution.make_random(ok_small, rng)
    ils = IteratedLocalSearch(ok_small, pm, rng, ls, init)

    # Search is a no-op, so we should return the initial solution after 10
    # iterations.
    result = ils.run(MaxIterations(10), collect_stats=True)
    assert_equal(result.best, init)
    assert_equal(result.num_iterations, 10)
    assert_equal(result.stats.num_iterations, 10)

    datum = result.stats.data[0]
    assert_equal(datum.current_cost, pm.cost_evaluator().penalised_cost(init))
    assert_equal(datum.current_feas, init.is_feasible())


def test_ils_acceptance_behaviour(ok_small):
    """
    Tests ILS acceptance behaviour over a fixed trajectory.
    """
    sols = [
        Solution(ok_small, [[1, 3], [2, 4]]),  # 22065 (infeas)
        Solution(ok_small, [[1, 2], [3, 4]]),  # 9725
        Solution(ok_small, [[1, 2], [4, 3]]),  # 9868
        Solution(ok_small, [[1, 4], [2, 3]]),  # 9240
        Solution(ok_small, [[1, 2], [3, 4]]),  # 9725
    ]

    ils = IteratedLocalSearch(
        ok_small,
        PenaltyManager(initial_penalties=([20], 6, 6)),
        RandomNumberGenerator(42),
        lambda *_, **kws: sols.pop(0),  # returns from sols one at a time
        sols[0],
        IteratedLocalSearchParams(history_length=2, exhaustive_on_best=False),
    )

    res = ils.run(stop=MaxIterations(len(sols)))
    data = res.stats.data

    # First solution is also the initial solution, so nothing should have
    # changed.
    assert_equal(data[0].current_cost, 22_065)
    assert_equal(data[0].candidate_cost, 22_065)
    assert_equal(data[0].best_cost, 22_065)

    # The second iteration has a solution that is *much* better. This new
    # solution should become both the current and best.
    assert_equal(data[1].current_cost, 9_725)
    assert_equal(data[1].candidate_cost, 9_725)
    assert_equal(data[1].best_cost, 9_725)

    # We now get a candidate solution that is a little worse than the previous
    # iteration, but better than the solution from two iterations ago. It is
    # thus accepted.
    assert_equal(data[2].current_cost, 9_868)
    assert_equal(data[2].candidate_cost, 9_868)
    assert_equal(data[2].best_cost, 9_725)

    # We now find a new best solution that should also be accepted.
    assert_equal(data[3].current_cost, 9_240)
    assert_equal(data[3].candidate_cost, 9_240)
    assert_equal(data[3].best_cost, 9_240)

    # In the last iteration we again find a solution we found earlier. This
    # solution improves over the solution from two iterations ago, and should
    # thus be accepted.
    assert_equal(data[4].current_cost, 9_725)
    assert_equal(data[4].candidate_cost, 9_725)
    assert_equal(data[4].best_cost, 9_240)


def test_restart(ok_small):
    """
    Tests that restarting clears the history of recent solutions and starts
    over from scratch.
    """
    Params = IteratedLocalSearchParams

    sols = [
        Solution(ok_small, [[1, 3], [2, 4]]),  # 22065 (infeas)
        Solution(ok_small, [[1, 2], [3, 4]]),  # 9725
        Solution(ok_small, [[1, 2], [3, 4]]),  # 9725
        Solution(ok_small, [[1, 2], [4, 3]]),  # 9868
    ]

    def run(params: IteratedLocalSearchParams):  # run with given params
        idx = iter(range(len(sols)))
        ils = IteratedLocalSearch(
            ok_small,
            PenaltyManager(initial_penalties=([20], 6, 6)),
            RandomNumberGenerator(42),
            lambda *_, **kw: sols[next(idx)],  # returns sols one at a time
            sols[0],
            params,
        )

        res = ils.run(MaxIterations(len(sols)))
        return res.stats.data

    # First run without a restart. We look back to the current solution from
    # last iteration, which means that we only accept improving solutions.
    curr_costs = [22065, 9725, 9725, 9725]
    params = Params(history_length=1, exhaustive_on_best=False)
    assert_equal([datum.current_cost for datum in run(params)], curr_costs)

    # But here a restart occurs in the third iteration, and that should clear
    # the history. We now accept the worsening solution in the last iteration.
    params = Params(
        num_iters_no_improvement=1,
        history_length=1,
        exhaustive_on_best=False,
    )
    curr_costs = [22065, 9725, 9725, 9868]
    assert_equal([datum.current_cost for datum in run(params)], curr_costs)


def test_exhaustive_search_on_new_best_solution(ok_small):
    """
    Tests that ILS calls the search method for an exhaustive evaluation on
    newfound best solutions.
    """
    sols = [
        Solution(ok_small, [[1, 2], [4, 3]]),  # 9868
        Solution(ok_small, [[1, 2], [3, 4]]),  # 9725 - new best
        Solution(ok_small, [[1, 4], [2, 3]]),  # 9240 - after exhaustive search
    ]

    def search(*_, exhaustive: bool):
        assert_(exhaustive if len(sols) == 1 else not exhaustive)
        return sols.pop(0)

    ils = IteratedLocalSearch(
        ok_small,
        PenaltyManager(initial_penalties=([20], 6, 6)),
        RandomNumberGenerator(42),
        search,
        sols[0],
    )

    # Just two iterations, but we should pop all three solutions, because the
    # second iteration again calls into the search method, but this time for
    # an exhaustive search that returns the final solution.
    res = ils.run(MaxIterations(2))
    assert_equal(len(sols), 0)
    assert_equal(res.cost(), 9_240)

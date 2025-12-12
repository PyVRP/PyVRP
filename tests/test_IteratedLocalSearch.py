from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import (
    IteratedLocalSearch,
    IteratedLocalSearchParams,
    PenaltyManager,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.IteratedLocalSearch import History
from pyvrp.search import Exchange10, LocalSearch, compute_neighbours
from pyvrp.stop import MaxIterations
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


def test_history():
    """
    Tests that the history correctly tracks recently inserted values, up to a
    fixed size, and can be cleared to reset its state.
    """
    history = History(size=2)
    assert_equal(len(history), 0)

    # Insert a single value, and test that the length, min, and mean values
    # are correct.
    history.append(1)
    assert_equal(len(history), 1)
    assert_equal(history.min(), 1)
    assert_equal(history.mean(), 1)

    # We now have two values, [1, 3]. min is still 1, but mean is now 2.
    history.append(3)
    assert_equal(len(history), 2)
    assert_equal(history.min(), 1)
    assert_equal(history.mean(), 2)

    # We now have three values, [1, 3, 5]. But the history can only store two,
    # so it should forget about the oldest value, 1. Thus, min is now 3, and
    # mean is 4.
    history.append(5)
    assert_equal(len(history), 2)
    assert_equal(history.min(), 3)
    assert_equal(history.mean(), 4)

    # Clearing the history class should reset its entire state.
    history.clear()
    assert_equal(len(history), 0)


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
        lambda *_: sols.pop(0),  # returns from sols one at a time
        sols[0],
    )

    res = ils.run(stop=MaxIterations(len(sols)))
    data = res.stats.data

    # First solution is also the initial solution, so nothing should have
    # changed.
    assert_equal(data[0].current_cost, 22_065)
    assert_equal(data[0].candidate_cost, 22_065)
    assert_equal(data[0].best_cost, 22_065)

    # The second iteration has a solution that is *much* better. This should
    # be accepted, and, since it's feasible, it should also be the new best
    # solution at the end of this iteration.
    assert_equal(data[1].current_cost, 9_725)
    assert_equal(data[1].candidate_cost, 9_725)
    assert_equal(data[1].best_cost, 9_725)

    # We now get a candidate solution that is a little worse than the previous
    # iteration, but still good enough to be accepted.
    assert_equal(data[2].current_cost, 9_868)
    assert_equal(data[2].candidate_cost, 9_868)
    assert_equal(data[2].best_cost, 9_725)

    # We now find a new best solution that should also be accepted.
    assert_equal(data[3].current_cost, 9_240)
    assert_equal(data[3].candidate_cost, 9_240)
    assert_equal(data[3].best_cost, 9_240)

    # In the last iteration we again find a solution we found earlier. But
    # since we are now at the end of our runtime budget, we no longer accept
    # worse solutions.
    assert_equal(data[4].current_cost, 9_240)
    assert_equal(data[4].candidate_cost, 9_725)
    assert_equal(data[4].best_cost, 9_240)

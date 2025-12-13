from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import (
    CostEvaluator,
    RandomNumberGenerator,
    Result,
    Solution,
    Statistics,
)
from pyvrp.ProgressPrinter import ProgressPrinter


def test_raises_negative_display_interval():
    """
    Tests that ProgressPrinter raises when provided with a negative
    display_interval argument.
    """
    with assert_raises(ValueError):
        ProgressPrinter(True, -1.0)


def test_start(ok_small, caplog):
    """
    Tests that the progress printer outputs some statistics about the given
    instance when calling start.
    """
    printer = ProgressPrinter(should_print=True, display_interval=1.0)
    printer.start(ok_small)

    out = caplog.text
    assert_(f"{ok_small.num_depots} depot" in out)
    assert_(f"{ok_small.num_clients} clients" in out)
    assert_(f"{ok_small.num_vehicles} vehicles" in out)


def test_start_multiple_depot_plural(ok_small_multi_depot, caplog):
    """
    Tests that "depots" is plural when there are multiple depots.
    """
    printer = ProgressPrinter(should_print=True, display_interval=1.0)
    printer.start(ok_small_multi_depot)

    assert_(f"{ok_small_multi_depot.num_depots} depots" in caplog.text)


def test_end(ok_small, caplog):
    """
    Tests that the progress printer outputs some summary statistics about the
    best-found solution and solver run when calling end.
    """
    best = Solution(ok_small, [[1, 2], [3, 4]])
    res = Result(best, Statistics(), 25, 0.05)

    printer = ProgressPrinter(should_print=True, display_interval=1.0)
    printer.end(res)

    out = caplog.text
    assert_(str(round(res.cost())) in out)
    assert_(str(res.num_iterations) in out)
    assert_(str(round(res.runtime)) in out)

    for line in res.summary().splitlines():
        assert_(line in out)


def test_restart(caplog):
    """
    Tests that calling restart outputs a line indicating such.
    """
    printer = ProgressPrinter(should_print=True, display_interval=1.0)
    printer.restart()

    out = caplog.text
    assert_("R" in out)
    assert_("restart" in out)


def test_iteration(ok_small, caplog):
    """
    Tests that calling iteration prints a line about the solution costs and
    their feasibility.
    """
    curr = Solution(ok_small, [[1, 2, 3, 4]])
    cand = Solution(ok_small, [[2, 1], [4, 3]])
    best = Solution(ok_small, [[1, 2], [3, 4]])
    cost_eval = CostEvaluator([20], 6, 6)

    stats = Statistics()
    stats.collect(cand, curr, best, cost_eval)

    printer = ProgressPrinter(should_print=True, display_interval=0.0)

    printer.iteration(stats)
    out = caplog.text
    assert_equal(stats.num_iterations, 1)

    # Statistics about solver progress should be printed.
    assert_(str(stats.num_iterations) in out)
    assert_(str(round(sum(stats.runtimes))) in out)

    # Statistics about solution costs and their feasibility.
    assert_("28408  N" in out)  # current
    assert_("10012  Y" in out)  # candidate
    assert_("9725  Y" in out)  # best


def test_should_print_false_no_output(ok_small, caplog):
    """
    Tests that disabling printing works.
    """
    sol = Solution(ok_small, [[1, 2, 3, 4]])
    cost_eval = CostEvaluator([20], 6, 6)

    stats = Statistics()
    stats.collect(sol, sol, sol, cost_eval)

    # Set up the progress printer, call all its methods, and then check that
    # the output captured on stdout is empty.
    printer = ProgressPrinter(should_print=False, display_interval=0.0)
    printer.start(ok_small)
    printer.iteration(stats=stats)
    printer.restart()

    rng = RandomNumberGenerator(42)
    sol = Solution.make_random(ok_small, rng)
    printer.end(result=Result(sol, stats, 25, 0.05))

    assert_equal(caplog.text, "")

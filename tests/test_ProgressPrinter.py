from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import (
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


def test_start(ok_small, capsys):
    """
    Tests that the progress printer outputs some statistics about the given
    instance when calling start.
    """
    printer = ProgressPrinter(should_print=True, display_interval=1.0)
    printer.start(ok_small)

    out = capsys.readouterr().out
    assert_(f"{ok_small.num_depots} depot" in out)
    assert_(f"{ok_small.num_clients} clients" in out)
    assert_(f"{ok_small.num_vehicles} vehicles" in out)


def test_start_multiple_depot_plural(ok_small_multi_depot, capsys):
    """
    Tests that "depots" is plural when there are multiple depots.
    """
    printer = ProgressPrinter(should_print=True, display_interval=1.0)
    printer.start(ok_small_multi_depot)

    out = capsys.readouterr().out
    assert_(f"{ok_small_multi_depot.num_depots} depots" in out)


def test_end(ok_small, capsys):
    """
    Tests that the progress printer outputs some summary statistics about the
    best-found solution and solver run when calling end.
    """
    best = Solution(ok_small, [[1, 2], [3, 4]])
    res = Result(best, Statistics(), 25, 0.05)

    printer = ProgressPrinter(should_print=True, display_interval=1.0)
    printer.end(res)

    out = capsys.readouterr().out
    assert_(str(round(res.cost())) in out)
    assert_(str(res.num_iterations) in out)
    assert_(str(round(res.runtime)) in out)
    assert_(res.summary() in out)


def test_restart(capsys):
    """
    Tests that calling restart outputs a line indicating such.
    """
    printer = ProgressPrinter(should_print=True, display_interval=1.0)
    printer.restart()

    out = capsys.readouterr().out
    assert_("R" in out)
    assert_("restart" in out)


def test_iteration(capsys):
    """
    Tests that calling iteration prints a line about the solution costs and
    their feasibility.
    """
    stats = Statistics()
    stats.collect(1, True, 2, True, 3, False)

    printer = ProgressPrinter(should_print=True, display_interval=0.0)

    printer.iteration(stats)
    out = capsys.readouterr().out
    assert_equal(stats.num_iterations, 1)

    # Statistics about solver progress should be printed.
    assert_(str(stats.num_iterations) in out)
    assert_(str(round(sum(stats.runtimes))) in out)

    # Statistics about solution costs and their feasibility.
    assert_("1 (Y)" in out)  # current
    assert_("2 (Y)" in out)  # candidate
    assert_("3 (N)" in out)  # best


def test_should_print_false_no_output(ok_small, capsys):
    """
    Tests that disabling printing works.
    """
    stats = Statistics()
    stats.collect(1, True, 2, True, 3, False)

    # Set up the progress printer, call all its methods, and then check that
    # the output captured on stdout is empty.
    printer = ProgressPrinter(should_print=False, display_interval=0.0)
    printer.start(ok_small)
    printer.iteration(stats=stats)
    printer.restart()

    rng = RandomNumberGenerator(42)
    sol = Solution.make_random(ok_small, rng)
    printer.end(result=Result(sol, stats, 25, 0.05))

    out = capsys.readouterr().out
    assert_equal(out, "")

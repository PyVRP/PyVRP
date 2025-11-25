import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import (
    CostEvaluator,
    Population,
    RandomNumberGenerator,
    Result,
    Solution,
    Statistics,
)
from pyvrp.ProgressPrinter import ProgressPrinter
from pyvrp.diversity import broken_pairs_distance as bpd


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
    Tests that calling iteration prints a line about the current feasible and
    infeasible populations.
    """
    pop = Population(bpd)
    rng = RandomNumberGenerator(seed=42)
    cost_eval = CostEvaluator([1], 1, 0)

    for _ in range(10):
        pop.add(Solution.make_random(ok_small, rng), cost_eval)

    assert_(pop.num_feasible() > 0)
    assert_(pop.num_infeasible() > 0)

    stats = Statistics()
    stats.collect_from(pop, cost_eval)

    printer = ProgressPrinter(should_print=True, display_interval=0.0)

    printer.iteration(stats)
    out = caplog.text
    assert_(stats.num_iterations, 1)

    # Statistics about solver progress should be printed.
    assert_(str(stats.num_iterations) in out)
    assert_(str(round(sum(stats.runtimes))) in out)

    # Statistics about feasible population.
    feas = stats.feas_stats[-1]
    assert_(str(feas.size) in out)
    assert_(str(round(feas.avg_cost)) in out)
    assert_(str(round(feas.best_cost)) in out)

    # Statistics about infeasible population.
    infeas = stats.infeas_stats[-1]
    assert_(str(infeas.size) in out)
    assert_(str(round(infeas.avg_cost)) in out)
    assert_(str(round(infeas.best_cost)) in out)


def test_should_print_false_no_output(ok_small, caplog):
    """
    Tests that disabling printing works.
    """
    pop = Population(bpd)
    rng = RandomNumberGenerator(seed=42)
    cost_eval = CostEvaluator([1], 1, 0)

    for _ in range(10):
        pop.add(Solution.make_random(ok_small, rng), cost_eval)

    best = Solution(ok_small, [[1, 2], [3, 4]])  # BKS
    pop.add(best, cost_eval)

    # Checks that there are both feasible and infeasible solutions in this
    # population.
    assert_(pop.num_feasible() > 0)
    assert_(pop.num_infeasible() > 0)

    stats = Statistics()
    stats.collect_from(pop, cost_eval)

    # Set up the progress printer, call all its methods, and then check that
    # the output captured on stdout is empty.
    printer = ProgressPrinter(should_print=False, display_interval=0.0)
    printer.start(ok_small)
    printer.iteration(stats=stats)
    printer.restart()
    printer.end(result=Result(best, stats, 25, 0.05))

    assert_equal(caplog.text, "")


@pytest.mark.parametrize(
    "routes",
    [
        [[1, 2], [3, 4]],  # feasible
        [[1, 2, 3, 4]],  # infeasible
    ],
)
def test_print_dash_when_subpopulation_is_empty(ok_small, routes, caplog):
    """
    Tests that a "-" is printed as cost if one of the subpopulations is empty.
    """
    pop = Population(bpd)
    cost_eval = CostEvaluator([1], 1, 0)

    solution = Solution(ok_small, routes)
    pop.add(solution, cost_eval)

    stats = Statistics()
    stats.collect_from(pop, cost_eval)

    printer = ProgressPrinter(should_print=True, display_interval=0.0)
    printer.iteration(stats)

    # Population contains either one feasible or one infeasible solution, so
    # the costs of the empty subpopulation should be printed as "-".
    assert_("-" in caplog.text)

from numpy.testing import assert_, assert_equal

from pyvrp.SolveParams import SolveParams
from pyvrp._pyvrp import PopulationParams
from pyvrp.solve import solve
from pyvrp.stop import MaxIterations


def test_solve_different_config(ok_small):
    """
    Tests that solving an instance with a custom configuration works as
    expected by checking that the population size is respected.
    """
    # First solve using the default configuration.
    res = solve(ok_small, stop=MaxIterations(200), seed=0)

    pop_params = PopulationParams()
    max_pop_size = pop_params.min_pop_size + pop_params.generation_size

    # Neither subpopulation should exceed the maximum population size;
    # we use the statistics as proxy to check this.
    max_feas_size = max([datum.size for datum in res.stats.feas_stats])
    max_infeas_size = max([datum.size for datum in res.stats.infeas_stats])

    assert_(max_feas_size <= max_pop_size)
    assert_(max_infeas_size <= max_pop_size)

    # Let's now use a custom confugration with a maximum population size of 15.
    pop_params = PopulationParams(min_pop_size=5, generation_size=10)
    config = SolveParams(population=pop_params)
    res = solve(ok_small, stop=MaxIterations(200), seed=0, config=config)

    max_pop_size = pop_params.min_pop_size + pop_params.generation_size
    max_feas_size = max([datum.size for datum in res.stats.feas_stats])
    max_infeas_size = max([datum.size for datum in res.stats.infeas_stats])

    assert_(max_feas_size <= max_pop_size)
    assert_(max_infeas_size <= max_pop_size)


def test_solve_display_argument(ok_small, capsys):
    """
    Tests that solving an instance displays solver progress when the
    ``display`` argument is ``True``.
    """
    # First solve with display turned off. We should not see any output in this
    # case.
    res = solve(ok_small, stop=MaxIterations(10), seed=0, display=False)
    printed = capsys.readouterr().out
    assert_equal(printed, "")

    # Now solve with display turned on. We should see output now.
    res = solve(ok_small, stop=MaxIterations(10), seed=0, display=True)
    printed = capsys.readouterr().out

    # Check that some of the header data is in the output.
    assert_("PyVRP" in printed)
    assert_("Time" in printed)
    assert_("Iters" in printed)
    assert_("Feasible" in printed)
    assert_("Infeasible" in printed)

    # Check that we include the cost and total runtime in the output somewhere.
    assert_(str(round(res.cost())) in printed)
    assert_(str(round(res.runtime)) in printed)


def test_solve_collect_stats(ok_small):
    """
    Tests that solving an instance with the ``collect_stats`` argument set to
    ``True`` collects statistics.
    """
    # Default is to collect statistics.
    res = solve(ok_small, stop=MaxIterations(10), seed=0)
    assert_(res.stats.is_collecting())

    # Now solve with statistics collection turned off.
    res = solve(ok_small, stop=MaxIterations(10), seed=0, collect_stats=False)
    assert_(not res.stats.is_collecting())

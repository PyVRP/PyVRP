from numpy.testing import assert_

from pyvrp.Config import Config
from pyvrp._pyvrp import PopulationParams
from pyvrp.solve import solve
from pyvrp.stop import MaxIterations


def test_solve_different_config(ok_small):
    """
    Tests that solving an instance with a custom configuration works as
    expected by checking that the population size is respected.
    """
    # First solve using the default configuration, which uses the default
    # population parameters.
    pop_params = PopulationParams()
    max_pop_size = pop_params.min_pop_size + pop_params.generation_size
    res = solve(ok_small, stop=MaxIterations(200), seed=0)

    max_feas_size = max([datum.size for datum in res.stats.feas_stats])
    max_infeas_size = max([datum.size for datum in res.stats.infeas_stats])

    assert_(max_feas_size <= max_pop_size)
    assert_(max_infeas_size <= max_pop_size)

    # Let's now use a custom configuration, where we modify the population
    # parameters to have a smaller maximum population size of 15.
    pop_params = PopulationParams(min_pop_size=5, generation_size=10)
    config = Config(population=pop_params)
    res = solve(ok_small, stop=MaxIterations(200), seed=0, config=config)

    max_feas_size = max([datum.size for datum in res.stats.feas_stats])
    max_infeas_size = max([datum.size for datum in res.stats.infeas_stats])

    assert_(max_feas_size <= max_pop_size)
    assert_(max_infeas_size <= max_pop_size)

from numpy.testing import assert_, assert_raises

from pyvrp import plotting
from pyvrp.Result import Result
from pyvrp.Statistics import Statistics
from pyvrp._lib.hgspy import Individual, PenaltyManager
from pyvrp.exceptions import StatisticsNotCollectedError
from pyvrp.tests.helpers import read


def test_plotting_methods_raise_when_no_stats_available():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    individual = Individual(data, pm, [[1, 2, 3, 4], [], []])
    res = Result(individual, Statistics(), 0, 0.0)

    assert_(not res.has_statistics())

    with assert_raises(StatisticsNotCollectedError):
        plotting.plot_result(res, data)

    with assert_raises(StatisticsNotCollectedError):
        plotting.plot_diversity(res)

    with assert_raises(StatisticsNotCollectedError):
        plotting.plot_objectives(res)

    with assert_raises(StatisticsNotCollectedError):
        plotting.plot_runtimes(res)

    # This one should not raise, since it does not depend on statistics.
    plotting.plot_solution(res.best, data)

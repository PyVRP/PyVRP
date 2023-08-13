import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import (
    CostEvaluator,
    Population,
    RandomNumberGenerator,
    Solution,
    Statistics,
)
from pyvrp.diversity import broken_pairs_distance
from pyvrp.tests.helpers import read


def test_csv_serialises_correctly(tmp_path):
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)
    pop = Population(broken_pairs_distance)

    collected_stats = Statistics()
    for _ in range(10):  # populate the statistics object
        pop.add(Solution.make_random(data, rng), cost_evaluator)
        collected_stats.collect_from(pop, cost_evaluator)

    csv_path = tmp_path / "test.csv"
    assert_(not csv_path.exists())

    # Write the collected statistcs to the CSV file location, and check that
    # the file now does exist.
    collected_stats.to_csv(csv_path)
    assert_(csv_path.exists())

    # Now read them back in, and check that the newly read object is the same
    # as the previously written object.
    read_stats = Statistics.from_csv(csv_path)
    assert_equal(collected_stats, read_stats)


@pytest.mark.parametrize("num_iterations", [0, 5])
def test_stats_collects_a_data_point_per_iteration(num_iterations: int):
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)
    pop = Population(broken_pairs_distance)

    stats = Statistics()
    for _ in range(num_iterations):  # populate the statistics object
        pop.add(Solution.make_random(data, rng), cost_evaluator)
        stats.collect_from(pop, cost_evaluator)

    assert_equal(len(stats.feas_stats), num_iterations)
    assert_equal(len(stats.infeas_stats), num_iterations)


@pytest.mark.parametrize("num_iterations", [0, 1, 10])
def test_eq(num_iterations: int):
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)
    pop = Population(broken_pairs_distance)

    stats = Statistics()
    for _ in range(num_iterations):  # populate the statistics object
        pop.add(Solution.make_random(data, rng), cost_evaluator)
        stats.collect_from(pop, cost_evaluator)

    assert_equal(stats, stats)
    assert_(stats != "test")

    if num_iterations > 0:
        assert_(stats != Statistics())

        # Tests equality for same objects. Part of that is that the NaN values
        # for data points of size == 0 are not compared.
        assert_equal(stats.feas_stats[0], stats.feas_stats[0])
        assert_equal(stats.infeas_stats[0], stats.infeas_stats[0])

        assert_(stats.feas_stats[0] != "test")
        assert_(stats.infeas_stats[0] != "test")


def test_more_eq():
    cost_evaluator = CostEvaluator(20, 6)
    pop = Population(broken_pairs_distance)
    assert_equal(len(pop), 0)

    stats1 = Statistics()
    stats2 = Statistics()

    assert_equal(stats1, stats2)
    assert_(stats1, stats1)
    assert_(stats1 != "str")

    stats1.collect_from(pop, cost_evaluator)
    assert_(stats1 != stats2)

    # Now do the same thing for stats2, so they have the seen the exact same
    # population trajectory. That, however, is not enough: the runtimes are
    # still slightly different.
    stats2.collect_from(pop, cost_evaluator)
    assert_(stats1 != stats2)

    # But once we fix that the two should be the exact same again.
    stats2.runtimes = stats1.runtimes
    assert_equal(stats1, stats2)

from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator, Population, RandomNumberGenerator, Statistics
from pyvrp.diversity import broken_pairs_distance
from pyvrp.tests.helpers import make_random_solutions, read


def test_csv_serialises_correctly(tmp_path):
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)
    pop = Population(broken_pairs_distance)

    for sol in make_random_solutions(10, data, rng):
        pop.add(sol, cost_evaluator)

    collected_stats = Statistics()

    for _ in range(10):  # populate the statistics object
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

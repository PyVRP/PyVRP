import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import Statistics


def test_csv_serialises_correctly(tmp_path):
    """
    Tests that writing a CSV of a ``Statistics`` object and then reading that
    CSV again returns the same object.
    """
    collected_stats = Statistics()
    for idx in range(10):  # populate the statistics object
        collected_stats.collect(idx, True, idx, True, idx, False)

    csv_path = tmp_path / "test.csv"
    assert_(not csv_path.exists())

    # Write the collected statistics to the CSV file location, and check that
    # the file now does exist.
    collected_stats.to_csv(csv_path)
    assert_(csv_path.exists())

    # Now read them back in, and check that the newly read object is the same
    # as the previously written object.
    read_stats = Statistics.from_csv(csv_path)
    assert_equal(collected_stats, read_stats)


@pytest.mark.parametrize("num_iterations", [0, 5])
def test_collect_a_data_point_per_iteration(num_iterations: int):
    """
    Tests that the statistics object collects solution statistics every time
    ``collect`` is called.
    """
    stats = Statistics()
    assert_(stats.is_collecting())

    for _ in range(num_iterations):  # populate the statistics object
        stats.collect(1, True, 2, True, 3, True)

    assert_equal(len(stats.data), num_iterations)


def test_data_point_matches_collect():
    """
    Tests that the data point collected matches with the values passed to
    ``collect()``.
    """
    stats = Statistics()
    stats.collect(1, True, 2, True, 3, False)

    datum = stats.data[0]

    assert_equal(datum.current_cost, 1)
    assert_(datum.current_feas)

    assert_equal(datum.candidate_cost, 2)
    assert_(datum.candidate_feas)

    assert_equal(datum.best_cost, 3)
    assert_(not datum.best_feas)


@pytest.mark.parametrize("num_iterations", [0, 1, 10])
def test_eq(num_iterations: int):
    """
    Tests the equality operator.
    """
    stats = Statistics()
    for _ in range(num_iterations):  # populate the statistics object
        stats.collect(1, True, 2, True, 3, True)

    assert_equal(stats, stats)
    assert_(stats != "test")

    if num_iterations > 0:
        assert_(stats != Statistics())


def test_more_eq():
    """
    Tests the equality operator for the same search trajectory.
    """
    stats1 = Statistics()
    stats2 = Statistics()

    assert_equal(stats1, stats2)
    assert_(stats1 != "str")

    stats1.collect(1, True, 2, True, 3, False)
    assert_(stats1 != stats2)

    # Now do the same thing for stats2, so they have the seen the exact same
    # search trajectory. That, however, is not enough: the runtimes are
    # still slightly different.
    stats2.collect(1, True, 2, True, 3, False)
    assert_(stats1 != stats2)

    # But once we fix that the two should be the exact same again.
    stats2.runtimes = stats1.runtimes
    assert_equal(stats1, stats2)


def test_not_collecting():
    """
    Tests that calling collect_from() on a Statistics object that is not
    collecting is a no-op.
    """
    stats = Statistics(collect_stats=False)
    assert_(not stats.is_collecting())

    stats.collect(1, True, 2, True, 3, False)
    stats.collect(1, True, 2, True, 3, False)

    assert_equal(stats, Statistics(collect_stats=False))

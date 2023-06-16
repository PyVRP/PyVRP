from numpy.testing import assert_equal

from pyvrp import CostEvaluator, Individual, XorShift128
from pyvrp.crossover import alternating_exchange as aex
from pyvrp.tests.helpers import read


def test_same_parents_same_offspring():
    """
    Tests that AEX produces identical offspring when both parents are the
    same.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    individual = Individual(data, [[1, 2], [3, 4]])
    offspring = aex((individual, individual), data, cost_evaluator, rng)

    assert_equal(offspring, individual)


def test_ignore_already_selected_clients():
    """
    Tests that AEX ignores clients that are already selected in either of
    the two parents.
    """
    data = read("data/OkSmallSplitTour.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    # 3 -> 2 -> 1 -> 3 (X) -> 4 -> 1 (X) -> 2 (X) -> 4 (X)
    parent1 = Individual(data, [[3, 1, 4, 2]])
    parent2 = Individual(data, [[2, 3, 1, 4]])

    offspring = aex((parent1, parent2), data, cost_evaluator, rng)
    expected = Individual(data, [[3, 2, 1, 4]])

    assert_equal(offspring, expected)


def test_perfect_split():
    data = read("data/OkSmallSplitTour.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    parent1 = Individual(data, [[3, 1], [2, 4]])
    parent2 = Individual(data, [[2, 4], [3, 1]])

    # AEX selects the clients in order of 3, 2, 1, 4. This is a feasible
    # route, so splitting the tour will result in a single identical route.
    offspring = aex((parent1, parent2), data, cost_evaluator, rng)
    expected = Individual(data, [[3, 2, 1, 4]])

    assert_equal(offspring, expected)


def test_split_into_multiple_routes():
    data = read("data/OkSmallSplitTour.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    parent1 = Individual(data, [[4, 2], [3, 1]])
    parent2 = Individual(data, [[3, 1], [4, 2]])

    # AEX selects the clients in order of 4, 3, 2, 1. The resulting split
    # is split into two routes [4] and [3, 2, 1].
    offspring = aex((parent1, parent2), data, cost_evaluator, rng)
    expected = Individual(data, [[4], [3, 2, 1]])

    assert_equal(offspring, expected)

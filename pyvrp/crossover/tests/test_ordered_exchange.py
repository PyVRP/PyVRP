from numpy.testing import assert_equal

from pyvrp import CostEvaluator, Solution, XorShift128
from pyvrp.crossover import ordered_exchange as ox
from pyvrp.crossover.ordered_exchange import (
    _ordered_exchange as deterministic_ox,
)
from pyvrp.tests.helpers import read


def test_same_feasible_parents_same_offspring():
    """
    Tests that AEX produces identical offspring when both parents are the
    same and feasible.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=42)

    solution = Solution(data, [[1, 2], [3, 4]])
    offspring = ox((solution, solution), data, cost_evaluator, rng)
    print(offspring)

    assert_equal(offspring, solution)

    # TODO maybe add the case where the offspring is not feasible


def test_():
    # Copy [start, end] inclusive
    data = read("data/OkSmallSplitTour.txt")

    tour1 = [1, 2, 3, 4]
    tour2 = [4, 3, 2, 1]

    # TODO I'm so confused about this
    offspring = deterministic_ox(tour1, tour2, data, (0, 0))
    assert_equal(offspring, tour2)

    offspring = deterministic_ox(tour1, tour2, data, (0, 3))
    assert_equal(offspring, tour1)

    offspring = deterministic_ox(tour1, tour2, data, (0, 1))
    assert_equal(offspring, [1, 2, 4, 3])

    offspring = deterministic_ox(tour1, tour2, data, (2, 3))
    assert_equal(offspring, [2, 1, 3, 4])

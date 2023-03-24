import numpy as np
from numpy.testing import assert_equal
from pytest import mark

from pyvrp import CostEvaluator, Individual
from pyvrp.tests.helpers import read


def test_load_penalty():
    cost_evaluator = CostEvaluator(2, 1)

    assert_equal(cost_evaluator.load_penalty(0, 1), 0)  # below capacity
    assert_equal(cost_evaluator.load_penalty(1, 1), 0)  # at capacity

    # Penalty per unit excess capacity is 2
    # 1 unit above capacity
    assert_equal(cost_evaluator.load_penalty(2, 1), 2)
    # 2 units above capacity
    assert_equal(cost_evaluator.load_penalty(3, 1), 4)

    # Penalty per unit excess capacity is 4
    cost_evaluator = CostEvaluator(4, 1)

    # 1 unit above capacity
    assert_equal(cost_evaluator.load_penalty(2, 1), 4)
    # 2 units above capacity
    assert_equal(cost_evaluator.load_penalty(3, 1), 8)


@mark.parametrize("capacity", [5, 15, 29, 51, 103])
def test_load_penalty_always_zero_when_below_capacity(capacity: int):
    load_penalty = 2
    cost_evaluator = CostEvaluator(load_penalty, 1)

    for load in range(capacity):  # all below capacity
        assert_equal(cost_evaluator.load_penalty(load, capacity), 0)

    # at capacity
    assert_equal(cost_evaluator.load_penalty(capacity, capacity), 0)
    # above capacity
    assert_equal(
        cost_evaluator.load_penalty(capacity + 1, capacity), load_penalty
    )
    assert_equal(
        cost_evaluator.load_penalty(capacity + 2, capacity), 2 * load_penalty
    )


def test_tw_penalty():
    cost_evaluator = CostEvaluator(1, 2)

    # Penalty per unit time warp is 2
    assert_equal(cost_evaluator.tw_penalty(0), 0)
    assert_equal(cost_evaluator.tw_penalty(1), 2)
    assert_equal(cost_evaluator.tw_penalty(2), 4)

    cost_evaluator = CostEvaluator(1, 4)

    # Penalty per unit excess capacity is now 4
    assert_equal(cost_evaluator.tw_penalty(0), 0)
    assert_equal(cost_evaluator.tw_penalty(1), 4)
    assert_equal(cost_evaluator.tw_penalty(2), 8)


@mark.parametrize(
    "routes",
    [[[1, 2], [3], [4]], [[1, 2, 3, 4]]],
)
def test_cost_and_penalized_cost(routes):
    data = read("data/OkSmall.txt")
    penalty_capacity = 20
    penalty_tw = 6
    default_cost_evaluator = CostEvaluator()
    cost_evaluator = CostEvaluator(penalty_capacity, penalty_tw)

    individual = Individual(data, routes)

    load_penalty_cost = penalty_capacity * individual.excess_load()
    tw_penalty_cost = penalty_tw * individual.time_warp()
    distance = individual.distance()

    # Test penalised cost
    expected_cost = distance + load_penalty_cost + tw_penalty_cost
    assert_equal(cost_evaluator.penalised_cost(individual), expected_cost)

    # Default cost evaluator has 0 weights and only computes distance as cost
    assert_equal(default_cost_evaluator.penalised_cost(individual), distance)

    # Test normal cost, should be distance or infinite if infeasible
    if individual.is_feasible():
        assert_equal(cost_evaluator.cost(individual), distance)
        assert_equal(default_cost_evaluator.cost(individual), distance)
    else:
        # C++ code represents infinite as max value for unsigned integer
        INFEAS_COST = np.iinfo(np.uint32).max
        assert_equal(cost_evaluator.cost(individual), INFEAS_COST)
        assert_equal(default_cost_evaluator.cost(individual), INFEAS_COST)

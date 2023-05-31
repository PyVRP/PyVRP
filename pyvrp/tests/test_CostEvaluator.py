import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal
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


def test_cost():
    data = read("data/OkSmall.txt")
    default_cost_evaluator = CostEvaluator()
    cost_evaluator = CostEvaluator(20, 6)

    # Feasible individual
    feas_indiv = Individual(data, [[1, 2], [3], [4]])
    distance = feas_indiv.distance()

    assert_equal(cost_evaluator.cost(feas_indiv), distance)
    assert_equal(default_cost_evaluator.cost(feas_indiv), distance)

    # Infeasible individual
    infeas_indiv = Individual(data, [[1, 2, 3, 4]])

    # C++ code represents infinity as UINT_MAX
    INFEAS_COST = np.iinfo(np.uint32).max
    assert_equal(cost_evaluator.cost(infeas_indiv), INFEAS_COST)
    assert_equal(default_cost_evaluator.cost(infeas_indiv), INFEAS_COST)


def test_cost_with_prizes():
    data = read("data/p06-2-50.vrp", round_func="dimacs")
    cost_evaluator = CostEvaluator(1, 1)

    indiv = Individual(data, [[1, 2], [3, 4, 5]])
    cost = cost_evaluator.cost(indiv)
    assert_(indiv.is_feasible())

    prizes = [data.client(idx).prize for idx in range(data.num_clients + 1)]
    collected = sum(prizes[:6])
    uncollected = sum(prizes) - collected

    assert_allclose(prizes[0], 0)
    assert_allclose(indiv.prizes(), collected)
    assert_allclose(indiv.uncollected_prizes(), uncollected)
    assert_allclose(indiv.distance() + indiv.uncollected_prizes(), cost)


def test_penalised_cost():
    data = read("data/OkSmall.txt")
    penalty_capacity = 20
    penalty_tw = 6
    default_cost_evaluator = CostEvaluator()
    cost_evaluator = CostEvaluator(penalty_capacity, penalty_tw)

    # Feasible individual
    feas_indiv = Individual(data, [[1, 2], [3], [4]])
    feas_dist = feas_indiv.distance()

    # For feasible individual, cost and penalised_cost should equal distance
    assert_equal(cost_evaluator.penalised_cost(feas_indiv), feas_dist)
    assert_equal(default_cost_evaluator.penalised_cost(feas_indiv), feas_dist)

    # Infeasible individual
    infeas_indiv = Individual(data, [[1, 2, 3, 4]])

    # Compute cost associated to violated constraints
    load_penalty_cost = penalty_capacity * infeas_indiv.excess_load()
    tw_penalty_cost = penalty_tw * infeas_indiv.time_warp()
    infeas_dist = infeas_indiv.distance()

    # Test penalised cost
    expected_cost = infeas_dist + load_penalty_cost + tw_penalty_cost
    assert_equal(cost_evaluator.penalised_cost(infeas_indiv), expected_cost)

    # Default cost evaluator has 0 weights and only computes distance as cost
    assert_equal(
        default_cost_evaluator.penalised_cost(infeas_indiv), infeas_dist
    )

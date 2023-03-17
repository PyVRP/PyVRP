from numpy.testing import assert_equal
from pytest import mark

from pyvrp._CostEvaluator import CostEvaluator


def test_load_penalty():
    cost_evaluator = CostEvaluator(2, 1)

    assert_equal(cost_evaluator.load_penalty(0, 1), 0)  # below capacity
    assert_equal(cost_evaluator.load_penalty(1, 1), 0)  # at capacity

    # Penalty per unit excess capacity is 2
    assert_equal(cost_evaluator.load_penalty(2, 1), 2)  # 1 unit above capacity
    assert_equal(
        cost_evaluator.load_penalty(3, 1), 4
    )  # 2 units above capacity

    # Penalty per unit excess capacity is 4
    cost_evaluator = CostEvaluator(4, 1)

    assert_equal(cost_evaluator.load_penalty(2, 1), 4)  # 1 unit above capacity
    assert_equal(
        cost_evaluator.load_penalty(3, 1), 8
    )  # 2 units above capacity


@mark.parametrize("capacity", [5, 15, 29, 51, 103])
def test_load_penalty_always_zero_when_below_capacity(capacity: int):
    load_penalty = 2
    cost_evaluator = CostEvaluator(load_penalty, 1)

    for load in range(capacity):  # all below capacity
        assert_equal(cost_evaluator.load_penalty(load, capacity), 0)

    assert_equal(
        cost_evaluator.load_penalty(capacity, capacity), 0
    )  # at capacity
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

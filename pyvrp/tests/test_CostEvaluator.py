import numpy as np
from numpy.testing import assert_, assert_allclose
from pytest import mark

from pyvrp import CostEvaluator, Solution
from pyvrp.tests.helpers import read


def test_load_penalty():
    """
    This test asserts that load penalty computations are correct.
    """
    cost_evaluator = CostEvaluator(2, 1)

    assert_allclose(cost_evaluator.load_penalty(0, 1), 0)  # below capacity
    assert_allclose(cost_evaluator.load_penalty(1, 1), 0)  # at capacity

    # Penalty per unit excess capacity is 2
    # 1 unit above capacity
    assert_allclose(cost_evaluator.load_penalty(2, 1), 2)
    # 2 units above capacity
    assert_allclose(cost_evaluator.load_penalty(3, 1), 4)

    # Penalty per unit excess capacity is 4
    cost_evaluator = CostEvaluator(4, 1)

    # 1 unit above capacity
    assert_allclose(cost_evaluator.load_penalty(2, 1), 4)
    # 2 units above capacity
    assert_allclose(cost_evaluator.load_penalty(3, 1), 8)


@mark.parametrize("capacity", [5, 15, 29, 51, 103])
def test_load_penalty_always_zero_when_below_capacity(capacity: int):
    """
    This test asserts that load penalties are only applied to excess load, that
    is, load in excess of the vehicle's capacity.
    """
    load_penalty = 2
    cost_evaluator = CostEvaluator(load_penalty, 1)

    for load in range(capacity):  # all below capacity
        assert_allclose(cost_evaluator.load_penalty(load, capacity), 0)

    # at capacity
    assert_allclose(cost_evaluator.load_penalty(capacity, capacity), 0)
    # above capacity
    assert_allclose(
        cost_evaluator.load_penalty(capacity + 1, capacity), load_penalty
    )
    assert_allclose(
        cost_evaluator.load_penalty(capacity + 2, capacity), 2 * load_penalty
    )


def test_tw_penalty():
    """
    This test asserts that time window penalty computations are correct.
    """
    cost_evaluator = CostEvaluator(1, 2)

    # Penalty per unit time warp is 2
    assert_allclose(cost_evaluator.tw_penalty(0), 0)
    assert_allclose(cost_evaluator.tw_penalty(1), 2)
    assert_allclose(cost_evaluator.tw_penalty(2), 4)

    cost_evaluator = CostEvaluator(1, 4)

    # Penalty per unit excess capacity is now 4
    assert_allclose(cost_evaluator.tw_penalty(0), 0)
    assert_allclose(cost_evaluator.tw_penalty(1), 4)
    assert_allclose(cost_evaluator.tw_penalty(2), 8)


def test_cost():
    """
    This test asserts that the cost is computed correctly for feasible
    individuals, and is a large value (representing infinity) for infeasible
    individuals.
    """
    data = read("data/OkSmall.txt")
    default_cost_evaluator = CostEvaluator()
    cost_evaluator = CostEvaluator(20, 6)

    feas_sol = Solution(data, [[1, 2], [3], [4]])  # feasible solution
    distance = feas_sol.distance()

    assert_allclose(cost_evaluator.cost(feas_sol), distance)
    assert_allclose(default_cost_evaluator.cost(feas_sol), distance)

    infeas_sol = Solution(data, [[1, 2, 3, 4]])  # infeasible solution
    assert_(not infeas_sol.is_feasible())

    # The C++ code represents infinity using a relevant maximal value, which
    # depends on the precision type (double or integer).
    if isinstance(distance, int):
        INFEAS_COST = np.iinfo(np.int32).max
    else:
        INFEAS_COST = np.finfo(np.float64).max

    assert_allclose(cost_evaluator.cost(infeas_sol), INFEAS_COST)
    assert_allclose(default_cost_evaluator.cost(infeas_sol), INFEAS_COST)


def test_cost_with_prizes():
    """
    When solving a prize-collecting instance, the cost is equal to the distance
    plus a prize term.
    """
    data = read("data/p06-2-50.vrp", round_func="dimacs")
    cost_evaluator = CostEvaluator(1, 1)

    sol = Solution(data, [[1, 2], [3, 4, 5]])
    cost = cost_evaluator.cost(sol)
    assert_(sol.is_feasible())

    prizes = [client.prize for client in data.clients()]
    collected = sum(prizes[:5])
    uncollected = sum(prizes) - collected

    assert_allclose(sol.prizes(), collected)
    assert_allclose(sol.uncollected_prizes(), uncollected)
    assert_allclose(sol.distance() + sol.uncollected_prizes(), cost)


def test_penalised_cost():
    """
    The penalised cost represents the smoothed objective, where constraint
    violations are priced in using penalty terms. It can be computed for both
    feasible and infeasible individuals. In case of the former, it is equal
    to the actual cost: the penalty terms are all zero.
    """
    data = read("data/OkSmall.txt")
    penalty_capacity = 20
    penalty_tw = 6
    default_evaluator = CostEvaluator()
    cost_evaluator = CostEvaluator(penalty_capacity, penalty_tw)

    feas = Solution(data, [[1, 2], [3], [4]])
    assert_(feas.is_feasible())

    # For a feasible solution, cost and penalised_cost equal distance.
    assert_allclose(cost_evaluator.penalised_cost(feas), feas.distance())
    assert_allclose(default_evaluator.penalised_cost(feas), feas.distance())

    infeas = Solution(data, [[1, 2, 3, 4]])
    assert_(not infeas.is_feasible())

    # Compute cost associated with violated constraints.
    load_penalty_cost = penalty_capacity * infeas.excess_load()
    tw_penalty_cost = penalty_tw * infeas.time_warp()
    infeas_dist = infeas.distance()

    # Test penalised cost
    expected_cost = infeas_dist + load_penalty_cost + tw_penalty_cost
    assert_allclose(cost_evaluator.penalised_cost(infeas), expected_cost)

    # Default cost evaluator has 0 weights and only computes distance as cost
    assert_allclose(default_evaluator.penalised_cost(infeas), infeas_dist)

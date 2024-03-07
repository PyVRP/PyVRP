import numpy as np
from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import CostEvaluator, Route, Solution, VehicleType


def test_load_penalty():
    """
    This test asserts that load penalty computations are correct.
    """
    cost_evaluator = CostEvaluator(2, 1, 0)

    assert_equal(cost_evaluator.load_penalty(0, 1), 0)  # below capacity
    assert_equal(cost_evaluator.load_penalty(1, 1), 0)  # at capacity

    # Penalty per unit excess capacity is 2
    # 1 unit above capacity
    assert_equal(cost_evaluator.load_penalty(2, 1), 2)
    # 2 units above capacity
    assert_equal(cost_evaluator.load_penalty(3, 1), 4)

    # Penalty per unit excess capacity is 4
    cost_evaluator = CostEvaluator(4, 1, 0)

    # 1 unit above capacity
    assert_equal(cost_evaluator.load_penalty(2, 1), 4)
    # 2 units above capacity
    assert_equal(cost_evaluator.load_penalty(3, 1), 8)


@mark.parametrize("cap", [5, 15, 29, 51, 103])
def test_load_penalty_always_zero_when_below_capacity(cap: int):
    """
    This test asserts that load penalties are only applied to excess load, that
    is, load in excess of the vehicle's capacity.
    """
    penalty = 2
    cost_eval = CostEvaluator(penalty, 1, 0)

    assert_equal(cost_eval.load_penalty(0, cap), 0)  # below cap
    assert_equal(cost_eval.load_penalty(cap - 1, cap), 0)
    assert_equal(cost_eval.load_penalty(cap, cap), 0)  # at cap
    assert_equal(cost_eval.load_penalty(cap + 1, cap), penalty)  # above cap
    assert_equal(cost_eval.load_penalty(cap + 2, cap), 2 * penalty)


def test_tw_penalty():
    """
    This test asserts that time window penalty computations are correct.
    """
    cost_evaluator = CostEvaluator(1, 2, 0)

    # Penalty per unit time warp is 2.
    assert_equal(cost_evaluator.tw_penalty(0), 0)
    assert_equal(cost_evaluator.tw_penalty(1), 2)
    assert_equal(cost_evaluator.tw_penalty(2), 4)

    cost_evaluator = CostEvaluator(1, 4, 0)

    # Penalty per unit excess capacity is now 4.
    assert_equal(cost_evaluator.tw_penalty(0), 0)
    assert_equal(cost_evaluator.tw_penalty(1), 4)
    assert_equal(cost_evaluator.tw_penalty(2), 8)


def test_dist_penalty():
    """
    This test asserts that excess distance penalty computations are correct.
    """
    cost_eval = CostEvaluator(1, 1, 2)

    # Penalty per unit excess distance is 2.
    assert_equal(cost_eval.dist_penalty(-1, 0), 0)
    assert_equal(cost_eval.dist_penalty(0, 0), 0)
    assert_equal(cost_eval.dist_penalty(1, 0), 2)
    assert_equal(cost_eval.dist_penalty(2, 0), 4)

    cost_eval = CostEvaluator(1, 1, 4)

    # Penalty per unit excess capacity is now 4.
    assert_equal(cost_eval.dist_penalty(-1, 0), 0)
    assert_equal(cost_eval.dist_penalty(0, 0), 0)
    assert_equal(cost_eval.dist_penalty(1, 0), 4)
    assert_equal(cost_eval.dist_penalty(2, 0), 8)


def test_cost(ok_small):
    """
    This test asserts that the cost is computed correctly for feasible
    solutions, and is a large value (representing infinity) for infeasible
    solutions.
    """
    default_cost_evaluator = CostEvaluator(0, 0, 0)
    cost_evaluator = CostEvaluator(20, 6, 0)

    feas_sol = Solution(ok_small, [[1, 2], [3], [4]])  # feasible solution
    distance = feas_sol.distance()

    assert_equal(cost_evaluator.cost(feas_sol), distance)
    assert_equal(default_cost_evaluator.cost(feas_sol), distance)

    infeas_sol = Solution(ok_small, [[1, 2, 3, 4]])  # infeasible solution
    assert_(not infeas_sol.is_feasible())

    infeas_cost = np.iinfo(np.int64).max
    assert_equal(cost_evaluator.cost(infeas_sol), infeas_cost)
    assert_equal(default_cost_evaluator.cost(infeas_sol), infeas_cost)


def test_cost_with_prizes(prize_collecting):
    """
    When solving a prize-collecting instance, the cost is equal to the distance
    plus a prize term.
    """
    data = prize_collecting
    cost_evaluator = CostEvaluator(1, 1, 0)

    sol = Solution(data, [[1, 2], [3, 4, 5]])
    cost = cost_evaluator.cost(sol)
    assert_(sol.is_feasible())

    prizes = [client.prize for client in data.clients()]
    collected = sum(prizes[:5])
    uncollected = sum(prizes) - collected

    assert_equal(sol.prizes(), collected)
    assert_equal(sol.uncollected_prizes(), uncollected)
    assert_equal(sol.distance() + sol.uncollected_prizes(), cost)


def test_penalised_cost(ok_small):
    """
    The penalised cost represents the smoothed objective, where constraint
    violations are priced in using penalty terms. It can be computed for both
    feasible and infeasible solutions. In case of the former, it is equal
    to the actual cost: the penalty terms are all zero.
    """
    penalty_capacity = 20
    penalty_tw = 6
    default_evaluator = CostEvaluator(0, 0, 0)
    cost_evaluator = CostEvaluator(penalty_capacity, penalty_tw, 0)

    feas = Solution(ok_small, [[1, 2], [3], [4]])
    assert_(feas.is_feasible())

    # For a feasible solution, cost and penalised_cost equal distance.
    assert_equal(cost_evaluator.penalised_cost(feas), feas.distance())
    assert_equal(default_evaluator.penalised_cost(feas), feas.distance())

    infeas = Solution(ok_small, [[1, 2, 3, 4]])
    assert_(not infeas.is_feasible())

    # Compute cost associated with violated constraints.
    load_penalty_cost = penalty_capacity * infeas.excess_load()
    tw_penalty_cost = penalty_tw * infeas.time_warp()
    infeas_dist = infeas.distance()

    # Test penalised cost
    expected_cost = infeas_dist + load_penalty_cost + tw_penalty_cost
    assert_equal(cost_evaluator.penalised_cost(infeas), expected_cost)

    # Default cost evaluator has 0 weights and only computes distance as cost
    assert_equal(default_evaluator.penalised_cost(infeas), infeas_dist)


def test_excess_distance_penalised_cost(ok_small):
    """
    Tests that excess distance is properly penalised in the cost computations.
    """
    vehicle_type = VehicleType(3, capacity=10, max_distance=5_000)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    sol = Solution(data, [[1, 2], [3, 4]])
    assert_(not sol.is_feasible())

    routes = sol.routes()

    assert_equal(sol.distance(), 5501 + 4224)
    assert_equal(routes[0].distance(), 5501)
    assert_equal(routes[1].distance(), 4224)

    assert_equal(sol.excess_distance(), 501)
    assert_equal(routes[0].excess_distance(), 501)
    assert_equal(routes[1].excess_distance(), 0)

    cost_eval = CostEvaluator(0, 0, 10)
    assert_equal(cost_eval.penalised_cost(sol), 5501 + 4224 + 10 * 501)


@mark.parametrize(
    ("assignment", "expected"), [((0, 0), 0), ((0, 1), 10), ((1, 1), 20)]
)
def test_cost_with_fixed_vehicle_cost(
    ok_small, assignment: tuple[int, int], expected: int
):
    """
    Tests that the cost evaluator counts the fixed cost when determining the
    objective value of a solution.
    """
    # First vehicle type is free, second costs 10 per vehicle. The solution
    # should be able to track this.
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(2, capacity=10, fixed_cost=0),
            VehicleType(2, capacity=10, fixed_cost=10),
        ]
    )

    routes = [
        Route(data, [1, 2], assignment[0]),
        Route(data, [3, 4], assignment[1]),
    ]

    sol = Solution(data, routes)
    cost_eval = CostEvaluator(1, 1, 0)

    # Solution is feasible, so penalised cost and regular cost are equal. Both
    # should contain the fixed vehicle cost.
    assert_(sol.is_feasible())
    assert_equal(cost_eval.cost(sol), sol.distance() + expected)
    assert_equal(cost_eval.penalised_cost(sol), sol.distance() + expected)

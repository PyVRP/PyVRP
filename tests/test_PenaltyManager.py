from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import PenaltyManager, PenaltyParams, Solution, VehicleType


@mark.parametrize(
    (
        "init_load_penalty",
        "init_tw_penalty",
        "init_dist_penalty",
        "repair_booster",
        "solutions_between_updates",
        "penalty_increase",
        "penalty_decrease",
        "target_feasible",
    ),
    [
        (1, 1, 1, 1, 0, -1.0, 0.5, 0.5),  # -1 penalty increase
        (1, 1, 1, 1, 0, 0.5, 0.5, 0.5),  # 0.5 penalty increase
        (1, 1, 1, 1, 0, 1.5, -1, 0.5),  # -1 penalty decrease
        (1, 1, 1, 1, 0, 1.5, 2, 0.5),  # 2 penalty decrease
        (1, 1, 1, 1, 0, 1, 1, -1),  # -1 target feasible
        (1, 1, 1, 1, 0, 1, 1, 2),  # 2 target feasible
        (1, 1, 1, 0, 0, 1, 1, 1),  # 0 repair booster
    ],
)
def test_constructor_throws_when_arguments_invalid(
    init_load_penalty: int,
    init_tw_penalty: int,
    init_dist_penalty: int,
    repair_booster: int,
    solutions_between_updates: int,
    penalty_increase: float,
    penalty_decrease: float,
    target_feasible: float,
):
    """
    Tests that invalid arguments are not accepted.
    """
    with assert_raises(ValueError):
        PenaltyParams(
            init_load_penalty,
            init_tw_penalty,
            init_dist_penalty,
            repair_booster,
            solutions_between_updates,
            penalty_increase,
            penalty_decrease,
            target_feasible,
        )


def test_repair_booster():
    """
    Tests that the booster evaluator returns a cost evaluator object that
    penalises constraint violations much more severely.
    """
    params = PenaltyParams(1, 1, 1, 5, 1, 1, 1, 1)
    pm = PenaltyManager(params)

    cost_evaluator = pm.cost_evaluator()

    assert_equal(cost_evaluator.tw_penalty(1), 1)
    assert_equal(cost_evaluator.load_penalty(2, 1), 1)  # 1 unit above capacity

    # With the booster, the penalty values are multiplied by the
    # repairBooster term (x5 in this case).
    booster = pm.booster_cost_evaluator()
    assert_equal(booster.tw_penalty(1), 5)
    assert_equal(booster.tw_penalty(2), 10)

    assert_equal(booster.load_penalty(2, 1), 5)  # 1 unit above capacity
    assert_equal(booster.load_penalty(3, 1), 10)  # 2 units above capacity

    # Test that using booster did not affect normal cost_evaluator.
    assert_equal(cost_evaluator.tw_penalty(1), 1)
    assert_equal(cost_evaluator.load_penalty(2, 1), 1)  # 1 unit above capacity


def test_load_penalty_update_increase(ok_small):
    """
    Tests that the load violation penalty is increased when too many load
    infeasible solutions have been generated since the last update.
    """
    num_registrations = 4
    params = PenaltyParams(1, 1, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 1)

    feas = Solution(ok_small, [[1, 2]])
    infeas = Solution(ok_small, [[1, 2, 3]])

    assert_(not feas.has_excess_load())
    assert_(infeas.has_excess_load())

    for sol in [feas, infeas, feas, infeas]:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 1)

    # Below targetFeasible, so should increase the capacityPenalty by +1
    # (normally to 1.1 due to penaltyIncrease, but we should not end up at the
    # same int).
    for sol in [infeas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 2)

    # Now we start from a much bigger initial capacityPenalty. Here we want the
    # penalty to increase by 10% due to penaltyIncrease = 1.1, and +1 due to
    # double -> int.
    params = PenaltyParams(100, 1, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 100)
    for sol in [infeas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 111)


def test_load_penalty_update_decrease(ok_small):
    """
    Tests that the load violation penalty is decreased when sufficiently many
    load feasible solutions have been generated since the last update.
    """
    num_registrations = 4
    params = PenaltyParams(4, 1, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    feas = Solution(ok_small, [[1, 2]])
    infeas = Solution(ok_small, [[1, 2, 3]])

    assert_(not feas.has_excess_load())
    assert_(infeas.has_excess_load())

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 4)
    for sol in [feas, infeas, feas, infeas]:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 4)

    # Above targetFeasible, so should decrease the capacityPenalty to 90%, and
    # -1 from the bounds check. So 0.9 * 4 = 3.6, 3.6 - 1 = 2.6, (int) 2.6 = 2
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 2)

    # Now we start from a much bigger initial capacityPenalty. Here we want the
    # penalty to decrease by 10% due to penaltyDecrease = 0.9, and -1 due to
    # double -> int.
    params = PenaltyParams(100, 1, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 100)
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 89)

    # Test that the penalty cannot decrease beyond 1, its minimum value.
    params = PenaltyParams(1, 1, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 1)
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 1)


def test_time_warp_penalty_update_increase(ok_small):
    """
    Tests that the time warp violation penalty is increased when too many time
    window infeasible solutions have been generated since the last update.
    """
    num_registrations = 4
    params = PenaltyParams(1, 1, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    feas = Solution(ok_small, [[1, 2]])
    infeas = Solution(ok_small, [[1, 2, 3]])

    assert_(not feas.has_time_warp())
    assert_(infeas.has_time_warp())

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.cost_evaluator().tw_penalty(1), 1)
    for sol in [feas, infeas, feas, infeas]:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 1)

    # Below targetFeasible, so should increase the tw penalty by +1
    # (normally to 1.1 due to penaltyIncrease, but we should not end up at the
    # same int).
    for sol in [infeas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 2)

    # Now we start from a much bigger initial tw penalty. Here we want
    # the penalty to increase by 10% due to penaltyIncrease = 1.1, and +1 due
    # to double -> int.
    params = PenaltyParams(1, 100, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    assert_equal(pm.cost_evaluator().tw_penalty(1), 100)
    for sol in [infeas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 111)


def test_time_warp_penalty_update_decrease(ok_small):
    """
    Tests that the time warp violation penalty is decreased when sufficently
    many time window feasible solutions have been generated since the last
    update.
    """
    num_registrations = 4
    params = PenaltyParams(1, 4, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    feas = Solution(ok_small, [[1, 2]])
    infeas = Solution(ok_small, [[1, 2, 3]])

    assert_(not feas.has_time_warp())
    assert_(infeas.has_time_warp())

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.cost_evaluator().tw_penalty(1), 4)
    for sol in [feas, infeas, feas, infeas]:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 4)

    # Above targetFeasible, so should decrease the timeWarpPenalty to 90%, and
    # -1 from the bounds check. So 0.9 * 4 = 3.6, 3.6 - 1 = 2.6, (int) 2.6 = 2
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 2)

    # Now we start from a much bigger initial timeWarpCapacity. Here we want
    # the penalty to decrease by 10% due to penaltyDecrease = 0.9, and -1 due
    # to double -> int.
    params = PenaltyParams(1, 100, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    assert_equal(pm.cost_evaluator().tw_penalty(1), 100)
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 89)

    # Test that the penalty cannot decrease beyond 1, its minimum value.
    params = PenaltyParams(1, 1, 1, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    assert_equal(pm.cost_evaluator().tw_penalty(1), 1)
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 1)


def test_does_not_update_penalties_before_sufficient_registrations(ok_small):
    """
    Tests that updates only happen every ``num_registrations`` times, not every
    time a new value is registered.
    """
    vehicle_type = VehicleType(3, capacity=10, max_distance=6_000)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    num_registrations = 4
    params = PenaltyParams(4, 4, 4, 1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(params)

    feas = Solution(data, [[1, 2], [3, 4]])
    infeas = Solution(data, [[1, 2, 3, 4]])

    assert_(feas.is_feasible())
    assert_(not infeas.is_feasible())

    # Both have four initial penalty, and vehicle capacity is one.
    assert_equal(pm.cost_evaluator().tw_penalty(1), 4)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 4)
    assert_equal(pm.cost_evaluator().dist_penalty(2, 1), 4)

    # Register three times. We need at least four registrations before the
    # penalties are updated, so this should not change anything.
    for sol in [feas, infeas, feas]:
        pm.register(sol)
        assert_equal(pm.cost_evaluator().tw_penalty(1), 4)
        assert_equal(pm.cost_evaluator().load_penalty(2, 1), 4)
        assert_equal(pm.cost_evaluator().dist_penalty(1, 0), 4)

    # Register a fourth time. Now the penalties should change. Since there are
    # more feasible registrations than desired, the penalties should decrease.
    pm.register(feas)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1), 2)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 2)
    assert_equal(pm.cost_evaluator().dist_penalty(1, 0), 2)


def test_max_min_penalty(ok_small):
    """
    Tests that penalty parameters are clipped to [1, 100_000] when updating
    their values.
    """
    params = PenaltyParams(
        init_time_warp_penalty=100_000,
        solutions_between_updates=1,
        penalty_decrease=0,
        penalty_increase=2,
    )
    pm = PenaltyManager(params)

    # Initial penalty is 100_000, so one unit of time warp should be penalised
    # by that value.
    assert_equal(pm.cost_evaluator().tw_penalty(1), 100_000)

    infeas = Solution(ok_small, [[1, 2, 3, 4]])
    assert_(infeas.has_time_warp())

    # When we register an infeasible solution, normally the penalty should go
    # up by two times due to the penalty_increase parameter. But it's already
    # at the upper limit, and can thus not increase further.
    pm.register(infeas)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 100_000)

    feas = Solution(ok_small, [[1, 2], [3, 4]])
    assert_(not feas.has_time_warp())

    # But when we register a feasible solution, the time warp penalty parameter
    # should drop to zero due to the penalty_decrease parameter. But penalty
    # parameters cannot drop below one.
    pm.register(feas)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 1)

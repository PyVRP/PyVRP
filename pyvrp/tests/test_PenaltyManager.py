from numpy.testing import assert_equal, assert_raises
from pytest import mark

from pyvrp._lib.hgspy import PenaltyManager, PenaltyParams


@mark.parametrize(
    "init_load_penalty,"
    "init_tw_penalty,"
    "repair_booster,"
    "penalty_increase,"
    "penalty_decrease,"
    "target_feasible",
    [
        (1, 1, 1, -1.0, 0.5, 0.5),  # -1 penalty increase
        (1, 1, 1, 0.5, 0.5, 0.5),  # 0.5 penalty increase
        (1, 1, 1, 1.5, -1, 0.5),  # -1 penalty decrease
        (1, 1, 1, 1.5, 2, 0.5),  # 2 penalty decrease
        (1, 1, 1, 1, 1, -1),  # -1 target feasible
        (1, 1, 1, 1, 1, 2),  # 2 target feasible
        (1, 1, 0, 1, 1, 1),  # 0 repair booster
    ],
)
def test_constructor_throws_when_arguments_invalid(
    init_load_penalty: int,
    init_tw_penalty: int,
    repair_booster: int,
    penalty_increase: float,
    penalty_decrease: float,
    target_feasible: float,
):
    with assert_raises(ValueError):
        params = PenaltyParams(
            init_load_penalty,
            init_tw_penalty,
            repair_booster,
            penalty_increase,
            penalty_decrease,
            target_feasible,
        )

        PenaltyManager(1, params)


def test_load_penalty():
    params = PenaltyParams(2, 1, 1, 1, 1, 1)
    pm = PenaltyManager(1, params)

    assert_equal(pm.load_penalty(0), 0)  # below capacity
    assert_equal(pm.load_penalty(1), 0)  # at capacity

    # Penalty per unit excess capacity is 2
    assert_equal(pm.load_penalty(2), 2)  # 1 unit above capacity
    assert_equal(pm.load_penalty(3), 4)  # 2 units above capacity

    # Penalty per unit excess capacity is 4
    params = PenaltyParams(4, 1, 1, 1, 1, 1)
    pm = PenaltyManager(1, params)

    assert_equal(pm.load_penalty(2), 4)  # 1 unit above capacity
    assert_equal(pm.load_penalty(3), 8)  # 2 units above capacity


def test_tw_penalty():
    params = PenaltyParams(1, 2, 1, 1, 1, 1)
    pm = PenaltyManager(1, params)

    # Penalty per unit time warp is 2
    assert_equal(pm.tw_penalty(0), 0)
    assert_equal(pm.tw_penalty(1), 2)
    assert_equal(pm.tw_penalty(2), 4)

    params = PenaltyParams(1, 4, 1, 1, 1, 1)
    pm = PenaltyManager(1, params)

    # Penalty per unit excess capacity is now 4
    assert_equal(pm.tw_penalty(0), 0)
    assert_equal(pm.tw_penalty(1), 4)
    assert_equal(pm.tw_penalty(2), 8)


def test_repair_booster():
    # params = PenaltyParams(1, 1, 5, 1, 1, 1)
    # pm = PenaltyManager(1, params)
    #
    # assert_equal(pm.tw_penalty(1), 1)
    # assert_equal(pm.load_penalty(2), 1)  # 1 unit above capacity
    #
    # # While the booster lives, the penalty values are multiplied by the
    # # repairBooster term (x5 in this case).
    # booster = pm.get_penalty_booster()
    #
    # assert_equal(pm.tw_penalty(1), 5)
    # assert_equal(pm.tw_penalty(2), 10)
    #
    # assert_equal(pm.load_penalty(2), 5)  # 1 unit above capacity
    # assert_equal(pm.load_penalty(3), 10)  # 2 units above capacity
    #
    # del booster
    #
    # # Booster no longer in scope, so penalties should return to normal.
    # assert_equal(pm.tw_penalty(1), 1)
    # assert_equal(pm.load_penalty(2), 1)  # 1 unit above capacity
    pass  # TODO when we figure out what to do with the repair booster


def test_capacity_penalty_update_increase():
    params = PenaltyParams(1, 1, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.load_penalty(2), 1)
    pm.update_capacity_penalty(0.5)
    assert_equal(pm.load_penalty(2), 1)

    # Below targetFeasible, so should increase the capacityPenalty by +1
    # (normally to 1.1 due to penaltyIncrease, but we should not end up at the
    # same int).
    pm.update_capacity_penalty(0.4)
    assert_equal(pm.load_penalty(2), 2)

    # Now we start from a much bigger initial capacityPenalty. Here we want the
    # penalty to increase by 10% due to penaltyIncrease = 1.1, and +1 due to
    # double -> int.
    params = PenaltyParams(100, 1, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    assert_equal(pm.load_penalty(2), 100)
    pm.update_capacity_penalty(0.4)
    assert_equal(pm.load_penalty(2), 111)

    # Test that the penalty cannot increase beyond 1000, its maximum value.
    params = PenaltyParams(1000, 1, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    assert_equal(pm.load_penalty(2), 1000)
    pm.update_capacity_penalty(0.4)
    assert_equal(pm.load_penalty(2), 1000)


def test_capacity_penalty_update_decrease():
    params = PenaltyParams(4, 1, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.load_penalty(2), 4)
    pm.update_capacity_penalty(0.5)
    assert_equal(pm.load_penalty(2), 4)

    # Above targetFeasible, so should decrease the capacityPenalty to 90%, and
    # -1 from the bounds check. So 0.9 * 4 = 3.6, 3.6 - 1 = 2.6, (int) 2.6 = 2
    pm.update_capacity_penalty(0.6)
    assert_equal(pm.load_penalty(2), 2)

    # Now we start from a much bigger initial capacityPenalty. Here we want the
    # penalty to decrease by 10% due to penaltyDecrease = 0.9, and -1 due to
    # double -> int.
    params = PenaltyParams(100, 1, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    assert_equal(pm.load_penalty(2), 100)
    pm.update_capacity_penalty(0.6)
    assert_equal(pm.load_penalty(2), 89)

    # Test that the penalty cannot decrease beyond 1, its minimum value.
    params = PenaltyParams(1, 1, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    assert_equal(pm.load_penalty(2), 1)
    pm.update_capacity_penalty(0.6)
    assert_equal(pm.load_penalty(2), 1)


def test_time_warp_penalty_update_increase():
    params = PenaltyParams(1, 1, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.tw_penalty(1), 1)
    pm.update_time_warp_penalty(0.5)
    assert_equal(pm.tw_penalty(1), 1)

    # Below targetFeasible, so should increase the tw penalty by +1
    # (normally to 1.1 due to penaltyIncrease, but we should not end up at the
    # same int).
    pm.update_time_warp_penalty(0.4)
    assert_equal(pm.tw_penalty(1), 2)

    # Now we start from a much bigger initial tw penalty. Here we want
    # the penalty to increase by 10% due to penaltyIncrease = 1.1, and +1 due
    # to double -> int.
    params = PenaltyParams(1, 100, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    assert_equal(pm.tw_penalty(1), 100)
    pm.update_time_warp_penalty(0.4)
    assert_equal(pm.tw_penalty(1), 111)

    # Test that the penalty cannot increase beyond 1000, its maximum value.
    params = PenaltyParams(1, 1000, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    assert_equal(pm.tw_penalty(1), 1000)
    pm.update_time_warp_penalty(0.4)
    assert_equal(pm.tw_penalty(1), 1000)


def test_time_warp_penalty_update_decrease():
    params = PenaltyParams(1, 4, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.tw_penalty(1), 4)
    pm.update_time_warp_penalty(0.5)
    assert_equal(pm.tw_penalty(1), 4)

    # Above targetFeasible, so should decrease the timeWarPenalty to 90%, and
    # -1 from the bounds check. So 0.9 * 4 = 3.6, 3.6 - 1 = 2.6, (int) 2.6 = 2
    pm.update_time_warp_penalty(0.6)
    assert_equal(pm.tw_penalty(1), 2)

    # Now we start from a much bigger initial timeWarpCapacity. Here we want
    # the penalty to decrease by 10% due to penaltyDecrease = 0.9, and -1 due
    # to double -> int.
    params = PenaltyParams(1, 100, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    assert_equal(pm.tw_penalty(1), 100)
    pm.update_time_warp_penalty(0.6)
    assert_equal(pm.tw_penalty(1), 89)

    # Test that the penalty cannot decrease beyond 1, its minimum value.
    params = PenaltyParams(1, 1, 1, 1.1, 0.9, 0.5)
    pm = PenaltyManager(1, params)

    assert_equal(pm.tw_penalty(1), 1)
    pm.update_time_warp_penalty(0.6)
    assert_equal(pm.tw_penalty(1), 1)

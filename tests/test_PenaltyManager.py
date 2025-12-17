import tracemalloc

import numpy as np
import pytest
from numpy.testing import (
    assert_,
    assert_allclose,
    assert_equal,
    assert_raises,
    assert_warns,
)

from pyvrp import PenaltyManager, PenaltyParams, Solution, VehicleType
from pyvrp.exceptions import PenaltyBoundWarning


@pytest.mark.parametrize(
    (
        "repair_booster",
        "solutions_between_updates",
        "penalty_increase",
        "penalty_decrease",
        "target_feasible",
        "feas_tolerance",
        "min_penalty",
        "max_penalty",
    ),
    [
        (0, 1, 1, 1, 1, 0, 0, 0),  # 0 repair booster
        (1, -1, 1, 0.5, 0.5, 0, 0, 0),  # -1 solutions between updates
        (1, 0, 1, 0.5, 0.5, 0, 0, 0),  # 0 solutions between updates
        (1, 1, -1.0, 0.5, 0.5, 0, 0, 0),  # -1 penalty increase
        (1, 1, 0.5, 0.5, 0.5, 0, 0, 0),  # 0.5 penalty increase
        (1, 1, 1.5, -1, 0.5, 0, 0, 0),  # -1 penalty decrease
        (1, 1, 1.5, 2, 0.5, 0, 0, 0),  # 2 penalty decrease
        (1, 1, 1, 1, -1, 0, 0, 0),  # -1 target feasible
        (1, 1, 1, 1, 2, 0, 0, 0),  # 2 target feasible
        (1, 1, 1.5, 0.5, 0.5, -1, 0, 0),  # -1 feas tolerance
        (1, 1, 1.5, 0.5, 0.5, 2, 0, 0),  # 2 feas tolerance
        (1, 1, 1.5, 0.5, 0.5, 0, -1, 0),  # -1 min_penalty
        (1, 1, 1.5, 0.5, 0.5, 0, 2, 1),  # max_penalty < min_penalty
    ],
)
def test_constructor_throws_when_arguments_invalid(
    repair_booster: int,
    solutions_between_updates: int,
    penalty_increase: float,
    penalty_decrease: float,
    target_feasible: float,
    feas_tolerance: float,
    min_penalty: float,
    max_penalty: float,
):
    """
    Tests that invalid arguments are not accepted.
    """
    with assert_raises(ValueError):
        PenaltyParams(
            repair_booster,
            solutions_between_updates,
            penalty_increase,
            penalty_decrease,
            target_feasible,
            feas_tolerance,
            min_penalty,
            max_penalty,
        )


def test_repair_booster():
    """
    Tests that the booster evaluator returns a cost evaluator object that
    penalises constraint violations much more severely.
    """
    params = PenaltyParams(5, 1, 1, 1, 1)
    pm = PenaltyManager(([1], 1, 1), params)

    cost_evaluator = pm.cost_evaluator()

    assert_equal(cost_evaluator.tw_penalty(1), 1)
    assert_equal(cost_evaluator.load_penalty(2, 1, 0), 1)  # 1 unit above cap

    # With the booster, the penalty values are multiplied by the
    # repairBooster term (x5 in this case).
    booster = pm.booster_cost_evaluator()
    assert_equal(booster.tw_penalty(1), 5)
    assert_equal(booster.tw_penalty(2), 10)

    assert_equal(booster.load_penalty(2, 1, 0), 5)  # 1 unit above capacity
    assert_equal(booster.load_penalty(3, 1, 0), 10)  # 2 units above capacity

    # Test that using booster did not affect normal cost_evaluator.
    assert_equal(cost_evaluator.tw_penalty(1), 1)
    assert_equal(cost_evaluator.load_penalty(2, 1, 0), 1)  # 1 unit above cap


def test_load_penalty_update_increase(ok_small):
    """
    Tests that the load violation penalty is increased when too many load
    infeasible solutions have been generated since the last update.
    """
    num_registrations = 4
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([1], 1, 1), params)

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 1)

    feas = Solution(ok_small, [[1, 2]])
    infeas = Solution(ok_small, [[1, 2, 3]])

    assert_(not feas.has_excess_load())
    assert_(infeas.has_excess_load())

    for sol in [feas, infeas, feas, infeas]:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 1)

    # Below targetFeasible, so should increase the loadPenalty to 1.1 due to
    # penaltyIncrease, and then int((2 - 1) * 1.1) = 1.
    for sol in [infeas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 1)

    # Now we start from a much bigger initial loadPenalty. Here we want the
    # penalty to increase by 10% due to penaltyIncrease = 1.1.
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([100], 1, 1), params)

    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 100)
    for sol in [infeas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 110)


def test_load_penalty_update_decrease(ok_small):
    """
    Tests that the load violation penalty is decreased when sufficiently many
    load feasible solutions have been generated since the last update.
    """
    num_registrations = 4
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([4], 1, 1), params)

    feas = Solution(ok_small, [[1, 2]])
    infeas = Solution(ok_small, [[1, 2, 3]])

    assert_(not feas.has_excess_load())
    assert_(infeas.has_excess_load())

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 4)
    for sol in [feas, infeas, feas, infeas]:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 4)

    # Above targetFeasible, so should decrease the loadPenalty to 90%, and -1
    # from the bounds check. So 0.9 * 4 = 3.6, and int(3.6) = 3.
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 3)

    # Now we start from a much bigger initial loadPenalty. Here we want the
    # penalty to decrease by 10% due to penaltyDecrease = 0.9, and -1 due to
    # double -> int.
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([100], 1, 1), params)

    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 100)
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 90)

    # Test that the penalty cannot decrease beyond min_penalty.
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([0.1], 1, 1), params)

    assert_equal(pm.cost_evaluator().load_penalty(11, 1, 0), 1)
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().load_penalty(11, 1, 0), 1)


def test_time_warp_penalty_update_increase(ok_small):
    """
    Tests that the time warp violation penalty is increased when too many time
    window infeasible solutions have been generated since the last update.
    """
    num_registrations = 4
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([1], 1, 1), params)

    feas = Solution(ok_small, [[1, 2]])
    infeas = Solution(ok_small, [[1, 2, 3]])

    assert_(not feas.has_time_warp())
    assert_(infeas.has_time_warp())

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.cost_evaluator().tw_penalty(1), 1)
    for sol in [feas, infeas, feas, infeas]:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 1)

    # Below targetFeasible, so should increase the tw penalty to 1.1 due to
    # penaltyIncrease, and int(1 * 1.1) = 1.
    for sol in [infeas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 1)

    # Now we start from a much bigger initial tw penalty. Here we want the
    # penalty to increase by 10% due to penaltyIncrease = 1.1, and +1 due
    # to double -> int.
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([1], 100, 1), params)

    assert_equal(pm.cost_evaluator().tw_penalty(1), 100)
    for sol in [infeas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 110)


def test_time_warp_penalty_update_decrease(ok_small):
    """
    Tests that the time warp violation penalty is decreased when sufficiently
    many time window feasible solutions have been generated since the last
    update.
    """
    num_registrations = 4
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([1], 4, 1), params)

    feas = Solution(ok_small, [[1, 2]])
    infeas = Solution(ok_small, [[1, 2, 3]])

    assert_(not feas.has_time_warp())
    assert_(infeas.has_time_warp())

    # Within bandwidth, so penalty should not change.
    assert_equal(pm.cost_evaluator().tw_penalty(1), 4)
    for sol in [feas, infeas, feas, infeas]:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 4)

    # Above targetFeasible, so should decrease the twCapacity to 90%. So
    # 0.9 * 4 = 3.6, and int(3.6) = 3.
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 3)

    # Now we start from a much bigger initial twCapacity. Here we want the
    # penalty to decrease by 10% due to penaltyDecrease = 0.9, and -1 due
    # to double -> int.
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([1], 100, 1), params)

    assert_equal(pm.cost_evaluator().tw_penalty(1), 100)
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 90)

    # Test that the penalty cannot decrease beyond min_penalty.
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([1], 0.1, 1), params)

    assert_equal(pm.cost_evaluator().tw_penalty(10), 1)
    for sol in [feas] * num_registrations:
        pm.register(sol)
    assert_equal(pm.cost_evaluator().tw_penalty(10), 1)


def test_does_not_update_penalties_before_sufficient_registrations(ok_small):
    """
    Tests that updates only happen every ``num_registrations`` times, not every
    time a new value is registered.
    """
    vehicle_type = VehicleType(3, capacity=[10], max_distance=6_000)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    num_registrations = 4
    params = PenaltyParams(1, num_registrations, 1.1, 0.9, 0.5)
    pm = PenaltyManager(([4], 4, 4), params)

    feas = Solution(data, [[1, 2], [3, 4]])
    infeas = Solution(data, [[1, 2, 3, 4]])

    assert_(feas.is_feasible())
    assert_(not infeas.is_feasible())

    # Both have four initial penalty, and vehicle capacity is one.
    assert_equal(pm.cost_evaluator().tw_penalty(1), 4)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 4)
    assert_equal(pm.cost_evaluator().dist_penalty(2, 1), 4)

    # Register three times. We need at least four registrations before the
    # penalties are updated, so this should not change anything.
    for sol in [feas, infeas, feas]:
        pm.register(sol)
        assert_equal(pm.cost_evaluator().tw_penalty(1), 4)
        assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 4)
        assert_equal(pm.cost_evaluator().dist_penalty(1, 0), 4)

    # Register a fourth time. Now the penalties should change. Since there are
    # more feasible registrations than desired, the penalties should decrease.
    pm.register(feas)
    assert_equal(pm.cost_evaluator().load_penalty(2, 1, 0), 3)
    assert_equal(pm.cost_evaluator().tw_penalty(1), 3)
    assert_equal(pm.cost_evaluator().dist_penalty(1, 0), 3)


@pytest.mark.filterwarnings("ignore::pyvrp.exceptions.PenaltyBoundWarning")
def test_max_min_penalty(ok_small):
    """
    Tests that penalty parameters are clipped to [min_penalty, max_penalty]
    when updating their values.
    """
    params = PenaltyParams(
        solutions_between_updates=1,
        penalty_decrease=0,
        penalty_increase=2,
    )
    pm = PenaltyManager(([20], params.max_penalty, 6), params)

    # Initial penalty is max_penalty, so one unit of time warp should be
    # penalised by that value.
    assert_equal(pm.cost_evaluator().tw_penalty(1), params.max_penalty)

    infeas = Solution(ok_small, [[1, 2, 3, 4]])
    assert_(infeas.has_time_warp())

    # When we register an infeasible solution, normally the penalty should go
    # up by two times due to the penalty_increase parameter. But it's already
    # at the upper limit, and can thus not increase further.
    pm.register(infeas)
    assert_equal(pm.cost_evaluator().tw_penalty(1), params.max_penalty)

    feas = Solution(ok_small, [[1, 2], [3, 4]])
    assert_(not feas.has_time_warp())

    # But when we register a feasible solution, the time warp penalty parameter
    # should drop to zero due to the penalty_decrease parameter. But penalty
    # parameters cannot drop below min_penalty.
    pm.register(feas)
    assert_allclose(
        pm.cost_evaluator().tw_penalty(10), 10 * params.min_penalty
    )


def test_warns_max_penalty_value(ok_small):
    """
    Tests that a penalty parameter clipped to max_penalty raises a warning.
    This typically indicates a data issue that PyVRP is struggling with.
    """
    params = PenaltyParams(solutions_between_updates=1)
    initial = ([1], params.max_penalty, 1)
    pm = PenaltyManager(initial, params)
    assert_equal(pm.penalties(), initial)

    infeas = Solution(ok_small, [[1, 2, 3, 4]])
    assert_(infeas.has_time_warp())

    with assert_warns(PenaltyBoundWarning):
        pm.register(infeas)


def test_init_from_load_penalty_value(ok_small):
    """
    Tests that ``init_from()`` computes the correct initial load penalty value
    for the OkSmall instance.
    """
    pm = PenaltyManager.init_from(ok_small)
    cost_eval = pm.cost_evaluator()

    avg_cost = ok_small.distance_matrix(0).mean()
    avg_load = np.mean([c.delivery for c in ok_small.clients()])
    assert_equal(cost_eval.load_penalty(1, 0, 0), int(avg_cost / avg_load))


def test_init_from_tw_penalty_value(ok_small):
    """
    Tests that ``init_from()`` computes the correct initial time warp penalty
    value for a slightly modified OkSmall instance.
    """
    distances = ok_small.distance_matrix(0)
    data = ok_small.replace(distance_matrices=[2 * distances])

    pm = PenaltyManager.init_from(data)
    cost_eval = pm.cost_evaluator()

    avg_distance = data.distance_matrix(0).mean()
    avg_duration = data.duration_matrix(0).mean()
    assert_equal(cost_eval.tw_penalty(1), round(avg_distance / avg_duration))


def test_init_from_different_unit_costs(ok_small):
    """
    Tests that ``init_from()`` computes the correct initial time warp and
    distance penalty values when the vehicles have a unit cost function that
    involves both distance and duration.
    """
    orig_type = ok_small.vehicle_type(0)
    veh_type = orig_type.replace(unit_distance_cost=1, unit_duration_cost=10)
    data = ok_small.replace(vehicle_types=[veh_type])

    pm = PenaltyManager.init_from(data)
    cost_eval = pm.cost_evaluator()

    avg_cost = np.mean(data.distance_matrix(0) + 10 * data.duration_matrix(0))
    avg_distance = data.distance_matrix(0).mean()
    avg_duration = data.duration_matrix(0).mean()

    assert_equal(cost_eval.tw_penalty(1), round(avg_cost / avg_duration))
    assert_equal(cost_eval.dist_penalty(1, 0), round(avg_cost / avg_distance))


def test_init_clips_penalties():
    """
    Tests that the initial penalty values are clipped to the [min_penalty,
    max_penalty] range.
    """
    params = PenaltyParams()
    penalties = ([0], params.max_penalty + 1, 2)
    pm = PenaltyManager(initial_penalties=penalties, params=params)

    cost_eval = pm.cost_evaluator()
    assert_allclose(  # MIN
        cost_eval.load_penalty(10, 0, 0),
        10 * params.min_penalty,
    )
    assert_equal(cost_eval.tw_penalty(1), params.max_penalty)  # MAX
    assert_equal(cost_eval.dist_penalty(1, 0), 2)  # already OK, so unchanged


def test_init_from_multiple_load_penalties(ok_small_multiple_load):
    """
    Tests that init_from correctly sets up multiple, different load penalties,
    one for each load dimension.
    """
    pm = PenaltyManager.init_from(ok_small_multiple_load)
    load_penalties, *_ = pm.penalties()
    assert_equal(
        len(load_penalties),
        ok_small_multiple_load.num_load_dimensions,
    )

    # The first load dimension has 18 total demand. The second 5. The ratio of
    # the load penalties should reflect this difference.
    load_penalty1, load_penalty2 = load_penalties
    assert_allclose(load_penalty1 / load_penalty2, 5 / 18)


def test_init_from_peak_memory_usage(mdvrptw):
    """
    Tests that peak memory usage is within some multiple of the problem data.
    """
    data = mdvrptw
    matrix, *_ = data.distance_matrices()

    tracemalloc.start()
    PenaltyManager.init_from(data)
    _, peak = tracemalloc.get_traced_memory()
    tracemalloc.stop()

    # Peak memory usage should be around 2 times the size of a single matrix,
    # including some overhead.
    assert peak < matrix.nbytes * 2.5

    # Adding more profiles should not blow up memory usage.
    matrices = [matrix] * 5
    data = data.replace(distance_matrices=matrices, duration_matrices=matrices)

    tracemalloc.start()
    PenaltyManager.init_from(data)
    _, peak = tracemalloc.get_traced_memory()
    tracemalloc.stop()

    assert peak < matrix.nbytes * 2.5

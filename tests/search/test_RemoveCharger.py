from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import RemoveCharger
from pyvrp.search._search import Node
from tests.helpers import make_search_route


def test_cannot_evaluate_unassigned(small_electric):
    """
    Tests that RemoveCharger cannot evaluate unassigned client nodes.
    """
    op = RemoveCharger(small_electric)
    cost_eval = CostEvaluator([], 1, 1)
    assert_allclose(op.evaluate(Node("C0"), cost_eval), (0, False))


def test_no_adjacent_charger(small_electric):
    """
    Tests that there is nothing to do if U has no adjacent chargers.
    """
    route = make_search_route(small_electric, ["C0", "C1", "R0", "C2"])
    op = RemoveCharger(small_electric)

    delta_cost, should_apply = op.evaluate(route[1], CostEvaluator([], 0, 0))
    assert_(delta_cost > 0)  # sentinel: no move was found
    assert_(not should_apply)


def test_removes_best_adjacent_charger(small_electric):
    """
    Tests that RemoveCharger removes the best adjacent charger.
    """
    route = make_search_route(small_electric, ["R0", "C0", "R1", "C1"])
    assert_equal(route.distance(), 120)

    op = RemoveCharger(small_electric)
    cost_eval = CostEvaluator([], 0, 0)

    # Removing R0 results in distance 100, and removing R1 in 80. The latter is
    # better, and is thus the move that should be applied.
    assert_allclose(op.evaluate(route[2], cost_eval), (-40, True))

    op.apply(route[2])
    route.update()

    assert_equal(str(route), "R0 C0 C1")
    assert_equal(route.num_chargers(), 1)


def test_removes_charger_before(small_electric):
    """
    Tests that RemoveCharger can remove the charger before U.
    """
    route = make_search_route(small_electric, ["C0", "R1", "C1"])
    op = RemoveCharger(small_electric)

    assert_allclose(
        op.evaluate(route[3], CostEvaluator([], 0, 0)), (-40, True)
    )

    op.apply(route[3])
    route.update()
    assert_equal(str(route), "C0 C1")
    assert_equal(route.num_chargers(), 0)


def test_keeps_needed_charger(small_electric):
    """
    Tests that RemoveCharger does not remove a charger when the resulting
    excess charge outweighs the shorter route.
    """
    route = make_search_route(small_electric, ["C0", "C1", "C2", "R1"])
    assert_equal(route.excess_charge(), 0)

    # Removing the charger saves 20 distance, but brings 30 excess charge.
    op = RemoveCharger(small_electric)
    cost_eval = CostEvaluator([], 0, 10)
    assert_allclose(op.evaluate(route[3], cost_eval), (280, False))


def test_removes_neutral_charger(small_electric):
    """
    Tests that a charger that does not help and costs nothing is removed.
    """
    veh_type = small_electric.vehicle_type(0).replace(recharge_duration=0)
    data = small_electric.replace(vehicle_types=[veh_type])

    # R0 is exactly on the way between C0 and C1.
    route = make_search_route(data, ["C0", "R0", "C1"])
    op = RemoveCharger(data)

    delta_cost, should_apply = op.evaluate(route[1], CostEvaluator([], 0, 0))
    assert_allclose(delta_cost, 0)
    assert_(should_apply)


def test_supports(ok_small, small_electric):
    """
    Tests that RemoveCharger needs chargers and vehicles with batteries.
    """
    assert_(not RemoveCharger.supports(ok_small))
    assert_(RemoveCharger.supports(small_electric))

from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import Charger, CostEvaluator
from pyvrp.search import ReplaceCharger
from pyvrp.search._search import Node
from tests.helpers import make_search_route


def test_cannot_evaluate_unassigned(small_electric):
    """
    Tests that ReplaceCharger cannot evaluate unassigned client nodes.
    """
    op = ReplaceCharger(small_electric)
    cost_eval = CostEvaluator([], 1, 1)
    assert_allclose(op.evaluate(Node("C0"), cost_eval), (0, False))


def test_no_adjacent_charger(small_electric):
    """
    Tests that there is nothing to do if U has no adjacent chargers.
    """
    route = make_search_route(small_electric, ["C0", "C1", "R0", "C2"])
    op = ReplaceCharger(small_electric)
    assert_allclose(
        op.evaluate(route[1], CostEvaluator([], 0, 10)), (0, False)
    )


def test_replaces_charger_after_node(small_electric):
    """
    Tests that ReplaceCharger can replace the charger after U in place.
    """
    route = make_search_route(small_electric, ["C0", "R1", "C1", "C2"])
    op = ReplaceCharger(small_electric)
    cost_eval = CostEvaluator([], 0, 10)

    # The route has distance 120 and excess charge 20. R0 in place of R1 gets
    # this to distance 80 and excess charge 10.
    assert_allclose(op.evaluate(route[1], cost_eval), (-140, True))

    op.apply(route[1])
    route.update()

    assert_equal(str(route), "C0 R0 C1 C2")
    assert_equal(route.num_chargers(), 1)


def test_replaces_charger_before_node(small_electric):
    """
    Tests that ReplaceCharger can replace the charger before U in place.
    """
    route = make_search_route(small_electric, ["C0", "R1", "C1"])
    op = ReplaceCharger(small_electric)

    assert_allclose(
        op.evaluate(route[3], CostEvaluator([], 0, 0)), (-40, True)
    )

    op.apply(route[3])
    route.update()
    assert_equal(str(route), "C0 R0 C1")


def test_moves_charger_after_node(small_electric):
    """
    Tests that ReplaceCharger can move the charger before U to behind it.
    """
    route = make_search_route(small_electric, ["C0", "R1", "C1", "C2"])
    op = ReplaceCharger(small_electric)
    cost_eval = CostEvaluator([], 0, 10)

    # The best move is to have R1 after C1: this removes all excess charge, at
    # distance 100. The cost goes from 320 to 100.
    assert_allclose(op.evaluate(route[3], cost_eval), (-220, True))

    op.apply(route[3])
    route.update()

    assert_equal(str(route), "C0 C1 R1 C2")
    assert_equal(route.excess_charge(), 0)


def test_moves_charger_before_node(small_electric):
    """
    Tests that ReplaceCharger can move the charger after U to before it.
    """
    route = make_search_route(small_electric, ["C0", "C1", "R0", "C2"])
    op = ReplaceCharger(small_electric)
    cost_eval = CostEvaluator([], 0, 1)

    # The route has distance 100 and excess charge 10. With R0 before C1 this
    # becomes distance 80 and excess charge 10, which beats the 100 distance
    # and no excess charge of replacing R0 by R1.
    assert_allclose(op.evaluate(route[2], cost_eval), (-20, True))

    op.apply(route[2])
    route.update()
    assert_equal(str(route), "C0 R0 C1 C2")


def test_single_charger_can_change_sides(small_electric):
    """
    Tests that ReplaceCharger also works with just one charger, since it can
    then still evaluate the other side of U.
    """
    data = small_electric.replace(chargers=[Charger(4)])
    route = make_search_route(data, ["C0", "C1", "R0", "C2"])
    op = ReplaceCharger(data)

    assert_allclose(
        op.evaluate(route[2], CostEvaluator([], 0, 1)), (-20, True)
    )


def test_not_applied_if_not_improving(small_electric):
    """
    Tests that ReplaceCharger does not apply a move that is not improving.
    """
    route = make_search_route(small_electric, ["C0", "R0", "C1"])
    op = ReplaceCharger(small_electric)

    _, should_apply = op.evaluate(route[1], CostEvaluator([], 0, 10))
    assert_(not should_apply)


def test_between_two_chargers(small_electric):
    """
    Tests that ReplaceCharger only replaces in place if there is a charger on
    both sides of U.
    """
    route = make_search_route(small_electric, ["R1", "C0", "R1", "C1"])
    op = ReplaceCharger(small_electric)

    # Replacing the first R1 by R0 saves 60, and the second R1 saves 40.
    assert_allclose(
        op.evaluate(route[2], CostEvaluator([], 0, 0)), (-60, True)
    )

    op.apply(route[2])
    route.update()
    assert_equal(str(route), "R0 C0 R1 C1")


def test_supports(ok_small, small_electric):
    """
    Tests that ReplaceCharger needs chargers and vehicles with batteries.
    """
    assert_(not ReplaceCharger.supports(ok_small))
    assert_(ReplaceCharger.supports(small_electric))

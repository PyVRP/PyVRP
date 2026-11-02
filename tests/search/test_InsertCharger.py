from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp import Charger, CostEvaluator
from pyvrp.search import InsertCharger
from pyvrp.search._search import Node
from tests.helpers import make_search_route


def test_cannot_evaluate_unassigned(small_electric):
    """
    Tests that InsertCharger cannot evaluate unassigned client nodes.
    """
    op = InsertCharger(small_electric)
    cost_eval = CostEvaluator([], 0, 10)
    assert_allclose(op.evaluate(Node("C0"), cost_eval), (0, False))


def test_skips_routes_without_excess_charge(small_electric):
    """
    Tests that InsertCharger does not consider routes that can be completed on
    the current battery.
    """
    route = make_search_route(small_electric, ["C0"])
    assert_equal(route.excess_charge(), 0)

    op = InsertCharger(small_electric)
    cost_eval = CostEvaluator([], 0, 10)
    assert_allclose(op.evaluate(route[1], cost_eval), (0, False))


def test_inserts_best_charger(small_electric):
    """
    Tests that InsertCharger finds the best charger and side of U.
    """
    route = make_search_route(small_electric, ["C0", "C1", "C2"])
    op = InsertCharger(small_electric)
    cost_eval = CostEvaluator([], 0, 10)

    # The route has distance 80 and excess charge 30, so its cost is 380. The
    # best option around C0 is R0 after it, which is on the way to C1. That
    # results in cost 80 + 10 * 10 = 180, so the delta cost is -200.
    assert_allclose(op.evaluate(route[1], cost_eval), (-200, True))

    op.apply(route[1])
    route.update()

    assert_equal(str(route), "C0 R0 C1 C2")
    assert_equal(route.num_chargers(), 1)
    assert_equal(route.distance(), 80)
    assert_equal(route.excess_charge(), 10)


def test_excess_charge_without_distance_cost(small_electric):
    """
    Tests that excess charge is also penalised when the vehicle does not pay
    for distance, which means the route has no distance cost component.
    """
    veh_type = small_electric.vehicle_type(0).replace(unit_distance_cost=0)
    data = small_electric.replace(vehicle_types=[veh_type])

    route = make_search_route(data, ["C0", "C1", "C2"])
    assert_(not route.has_distance_cost())

    op = InsertCharger(data)
    cost_eval = CostEvaluator([], 0, 10)

    # Same as in test_inserts_best_charger, but with free distance. The excess
    # charge goes from 30 to 10, which saves 200 in penalties.
    assert_allclose(op.evaluate(route[1], cost_eval), (-200, True))


def test_inserts_before_node(small_electric):
    """
    Tests that InsertCharger inserts the charger before U if that is best.
    """
    route = make_search_route(small_electric, ["C2", "C1", "C0"])
    op = InsertCharger(small_electric)
    cost_eval = CostEvaluator([], 0, 10)

    # Same as above, but the route is reversed.
    assert_allclose(op.evaluate(route[3], cost_eval), (-200, True))

    op.apply(route[3])
    route.update()
    assert_equal(str(route), "C2 C1 R0 C0")


def test_delta_includes_recharge_duration(small_electric):
    """
    Tests that the delta cost accounts for the duration of the recharge.
    """
    veh_type = small_electric.vehicle_type(0).replace(unit_duration_cost=1)
    data = small_electric.replace(vehicle_types=[veh_type])

    route = make_search_route(data, ["C0", "C1", "C2"])
    op = InsertCharger(data)

    # As in test_inserts_best_charger, but now the recharge duration of 5 also
    # counts towards the objective.
    cost_eval = CostEvaluator([], 0, 10)
    assert_allclose(op.evaluate(route[1], cost_eval), (-195, True))


def test_not_applied_if_not_improving(small_electric):
    """
    Tests that InsertCharger does not apply a move that is not improving, which
    is the case here because the penalty on excess charge is zero.
    """
    route = make_search_route(small_electric, ["C0", "C1", "C2"])
    op = InsertCharger(small_electric)

    _, should_apply = op.evaluate(route[1], CostEvaluator([], 0, 0))
    assert_(not should_apply)


def test_can_insert_next_to_charger(small_electric):
    """
    Tests that InsertCharger can insert a charger next to another charger,
    which is sometimes needed to bridge the distance between clients.
    """
    # It is very far from C2 to the chargers, so the only useful place for
    # another charger is before C2, directly after R0.
    distances = small_electric.distance_matrix(0).copy()
    distances[3, [4, 5]] = 1_000

    veh_type = small_electric.vehicle_type(0).replace(battery_capacity=30)
    data = small_electric.replace(
        vehicle_types=[veh_type],
        distance_matrices=[distances],
    )

    # The stretch from R0 to the depot is 60, which is 30 too long.
    route = make_search_route(data, ["R0", "C2"])
    assert_equal(route.excess_charge(), 30)

    # R1 adds 20 distance and shortens the last stretch to 50.
    op = InsertCharger(data)
    cost_eval = CostEvaluator([], 0, 10)
    assert_allclose(op.evaluate(route[2], cost_eval), (20 - 100, True))

    op.apply(route[2])
    route.update()
    assert_equal(str(route), "R0 R1 C2")
    assert_equal(route.excess_charge(), 20)


def test_supports(ok_small, small_electric):
    """
    Tests that InsertCharger needs both chargers and vehicles with batteries.
    """
    assert_(not InsertCharger.supports(ok_small))
    assert_(InsertCharger.supports(small_electric))

    # Chargers, but no vehicle type that can use them.
    data = ok_small.replace(chargers=[Charger(0)])
    assert_(not InsertCharger.supports(data))

    # Vehicles with a battery, but no chargers.
    data = small_electric.replace(chargers=[])
    assert_(not InsertCharger.supports(data))

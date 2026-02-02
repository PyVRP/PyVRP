from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import RemoveAdjacentDepot
from pyvrp.search._search import Node
from tests.helpers import make_search_route


def test_cannot_evaluate_unassigned(ok_small_multiple_trips):
    """
    Tests that RemoveAdjacentDepot cannot evaluate unassigned client nodes.
    """
    data = ok_small_multiple_trips
    cost_eval = CostEvaluator([1], 1, 1)
    op = RemoveAdjacentDepot(data)

    unassigned = Node(loc=1)
    assert_equal(op.evaluate(unassigned, cost_eval), (0, False))


def test_removes_best_adjacent_depot(ok_small_multiple_trips):
    """
    Tests that RemoveAdjacentDepot removes the best adjacent reload depot.
    """
    veh_type = ok_small_multiple_trips.vehicle_type(0).replace(max_reloads=2)
    data = ok_small_multiple_trips.replace(vehicle_types=[veh_type])
    route = make_search_route(data, [1, 0, 2, 0, 3])

    cost_eval = CostEvaluator([0], 0, 0)
    op = RemoveAdjacentDepot(data)

    # Client 1 has a depot after it, that can be removed. Delta cost is purely
    # distance based:
    #   delta = dist(1, 2) - dist(1, 0) - dist(0, 2)
    #         = 1_992 - 1_726 - 1_944
    #         = -1_678.
    assert_equal(op.evaluate(route[1], cost_eval), (-1_678, True))

    # Client 2 has a depot before and after it. Removing the depot after it is
    # better, so that's the move we should apply.
    assert_equal(op.evaluate(route[3], cost_eval), (-3_275, True))

    # Client 3 has a depot before it. Removing that depot is the same move as
    # we evaluated for client 2, so we should find the same delta cost.
    assert_equal(op.evaluate(route[5], cost_eval), (-3_275, True))

    # Applying the last evaluated move removes the depot between 2 and 3.
    op.apply(route[5])
    assert_equal(str(route), "1 | 2 3")


def test_removes_consecutive_depots(ok_small_multiple_trips):
    """
    Tests that RemoveAdjacentDepot removes consecutive adjacent reload depots.
    """
    veh_type = ok_small_multiple_trips.vehicle_type(0).replace(max_reloads=2)
    data = ok_small_multiple_trips.replace(vehicle_types=[veh_type])
    route = make_search_route(data, [1, 0, 0, 2])

    cost_eval = CostEvaluator([0], 0, 0)
    op = RemoveAdjacentDepot(data)

    # The move is cost neutral, but should be applied anyway.
    delta_cost, should_apply = op.evaluate(route[1], cost_eval)
    assert_equal(delta_cost, 0)
    assert_(should_apply)


def test_supports(ok_small, ok_small_multiple_trips, mtvrptw_release_times):
    """
    Tests that RemoveAdjacentDepot supports instances with reloading.
    """
    assert_(not RemoveAdjacentDepot.supports(ok_small))
    assert_(RemoveAdjacentDepot.supports(ok_small_multiple_trips))
    assert_(RemoveAdjacentDepot.supports(mtvrptw_release_times))

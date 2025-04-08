from numpy.testing import assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import TripRelocate
from pyvrp.search._search import Node, Route


def test_inserts_depot_single_route(ok_small_multiple_trips):
    """
    Tests that TripRelocate inserts a reload depot along with the node
    relocation in the same route.
    """
    route = Route(ok_small_multiple_trips, 0, 0)
    for loc in [1, 2, 3, 4]:
        route.append(Node(loc=loc))
    route.update()

    assert_equal(str(route), "1 2 3 4")
    assert_equal(route.num_depots, 2)
    assert_equal(route.num_trips, 1)
    assert_equal(route.excess_load(), [8])

    op = TripRelocate(ok_small_multiple_trips)
    cost_eval = CostEvaluator([500], 0, 0)

    # The route now is 1 2 3 4, proposal evaluates 1 3 | 2 4 and 1 3 2 | 4. Of
    # these two moves, the move resulting in 1 3 | 2 4 is better, with total
    # route cost 9_543 (compared to 10_450 now). The cost delta is thus -907.
    assert_equal(op.evaluate(route[2], route[3], cost_eval), -907)

    op.apply(route[2], route[3])
    route.update()

    # There should now be an additional reload depot and trip, and all excess
    # load should have been resolved by the reloading.
    assert_equal(route.num_depots, 3)
    assert_equal(route.num_trips, 2)
    assert_equal(route.excess_load(), [0])

    # Check that the route now indeed includes the "3 | 2" bit.
    assert_equal(str(route), "1 3 | 2 4")


def test_inserts_depot_across_routes(ok_small_multiple_trips):
    """
    Tests that TripRelocate inserts a reload depot along with the node
    relocation across routes.
    """
    route1 = Route(ok_small_multiple_trips, 0, 0)
    route1.append(Node(loc=3))
    route1.update()

    route2 = Route(ok_small_multiple_trips, 1, 0)
    for loc in [1, 2, 4]:
        route2.append(Node(loc=loc))
    route2.update()

    assert_equal(str(route1), "3")
    assert_equal(str(route2), "1 2 4")

    op = TripRelocate(ok_small_multiple_trips)
    cost_eval = CostEvaluator([500], 0, 0)

    # The proposal evaluates 1 | 3 2 4 and 1 3 | 2 4. Of these, the second is
    # better, with total cost 9_543 (compared to 3_994 + 8_601 now). The cost
    # delta is thus -3_052.
    assert_equal(op.evaluate(route1[1], route2[1], cost_eval), -3_052)

    op.apply(route1[1], route2[1])
    route1.update()
    route2.update()

    assert_equal(str(route1), "")
    assert_equal(str(route2), "1 3 | 2 4")


def test_reload_depot_before_or_after_relocate():
    """
    TripRelocate evaluates placing a reload depot either before or after the
    relocated node. This test checks if it picks the best option.
    """
    pass  # TODO


def test_inserts_best_reload_depot():
    """
    Tests that TripRelocate inserts the best possible reload depot, not just
    the first improving one.
    """
    pass  # TODO

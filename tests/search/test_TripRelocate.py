import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator, Depot, ProblemData, VehicleType
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


@pytest.mark.parametrize(
    ("load_penalty", "exp_delta_cost", "exp_route_str"),
    [
        # With such a large load penalty, we insert the depot after 1 because
        # that ensures the route has no excess load, which dominates the cost
        # structure.
        (1_000, -3_897, "2 1 | 3 4"),
        # With this load penalty the time aspect is still important, and we
        # insert the depot before 1 because that is better w.r.t. time warp.
        (300, -54, "2 | 1 3 4"),  # depot before 1
    ],
)
def test_reload_depot_before_or_after_relocate(
    ok_small_multiple_trips,
    load_penalty: int,
    exp_delta_cost: int,
    exp_route_str: str,
):
    """
    TripRelocate evaluates placing a reload depot either before or after the
    relocated node. This test checks if it picks the best option.
    """
    route = Route(ok_small_multiple_trips, 0, 0)
    for loc in [1, 2, 3, 4]:
        route.append(Node(loc=loc))
    route.update()

    op = TripRelocate(ok_small_multiple_trips)
    cost_eval = CostEvaluator([load_penalty], 1, 0)
    assert_equal(op.evaluate(route[1], route[2], cost_eval), exp_delta_cost)

    op.apply(route[1], route[2])
    route.update()

    assert_equal(str(route), exp_route_str)


def test_inserts_best_reload_depot():
    """
    Tests that TripRelocate inserts the best possible reload depot, not just
    the first improving one.
    """
    # Only non-zero in and out of the first depot, so we do not want to use
    # that one - we instead prefer the second one, which is free.
    mat = np.zeros((4, 4), dtype=int)
    mat[0, 2:] = 100
    mat[2:, 0] = 100

    veh_type = VehicleType(capacity=[5], reload_depots=[0, 1], max_reloads=1)
    data = ProblemData(
        clients=[Client(0, 0, delivery=[5]), Client(0, 0, delivery=[5])],
        depots=[Depot(0, 0), Depot(0, 0)],
        vehicle_types=[veh_type],
        distance_matrices=[mat],
        duration_matrices=[mat],
    )

    route = Route(data, 0, 0)
    route.append(Node(loc=2))
    route.append(Node(loc=3))
    route.update()

    assert_(route.has_excess_load())
    assert_equal(route.excess_load(), [5])

    op = TripRelocate(data)
    cost_eval = CostEvaluator([500], 0, 0)

    # We evaluate two moves, 3 | 2 and 3 2 |, for each depot (0 and 1). Only
    # 3 | 2 removes excess load. Then the depot choice: depot 0 incurs a small
    # routing costs, whereas 1 is fee. Thus, we should evaluate and apply the
    # move using depot 1, at delta cost -2_500.
    assert_equal(op.evaluate(route[1], route[2], cost_eval), -2_500)

    op.apply(route[1], route[2])
    route.update()

    assert_(not route.has_excess_load())
    assert_equal(str(route), "3 | 2")
    assert_equal(route[2].client, 1)

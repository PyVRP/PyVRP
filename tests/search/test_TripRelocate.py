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
    assert_equal(route.num_depots(), 2)
    assert_equal(route.num_trips(), 1)
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
    assert_equal(route.num_depots(), 3)
    assert_equal(route.num_trips(), 2)
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

    veh_type = VehicleType(capacity=[5], reload_depots=[0, 1])
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


def test_fixed_vehicle_cost():
    """
    Tests that TripRelocate correctly accounts for fixed vehicle costs due to
    non-empty routes becoming empty.
    """
    data = ProblemData(
        clients=[Client(0, 0, delivery=[5]), Client(0, 0, delivery=[4])],
        depots=[Depot(0, 0)],
        vehicle_types=[
            VehicleType(
                num_available=2,
                capacity=[4],
                fixed_cost=2_000,
                reload_depots=[0],
                max_reloads=1,
            )
        ],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    route1 = Route(data, 0, 0)
    route1.append(Node(loc=1))
    route1.update()

    route2 = Route(data, 1, 0)
    route2.append(Node(loc=2))
    route2.update()

    op = TripRelocate(data)
    cost_eval = CostEvaluator([0], 0, 0)

    # After this move, route1 is empty, which results in a cost delta of -2000
    # because all other costs are zero.
    assert_equal(op.evaluate(route1[1], route2[1], cost_eval), -2_000)


def test_does_not_evaluate_if_already_max_trips(ok_small_multiple_trips):
    """
    Tests that TripRelocate does not evaluate moves that would result in more
    trips than the vehicle on the route can execute.
    """
    route = Route(ok_small_multiple_trips, 0, 0)
    for loc in [3, 0, 1, 2, 4]:
        route.append(Node(loc=loc))
    route.update()

    assert_equal(str(route), "3 | 1 2 4")
    assert_equal(route.excess_load(), [5])

    op = TripRelocate(ok_small_multiple_trips)
    cost_eval = CostEvaluator([10_000], 0, 0)

    # This move would result in either 3 | 2 | 1 4, or 3 | 2 1 | 4, both of
    # which would resolve any excess load. But that's more trips than the
    # vehicle can perform, so this move cannot be done.
    assert_equal(op.evaluate(route[3], route[4], cost_eval), 0)
    assert_equal(route.num_trips(), route.max_trips())


def test_trip_relocate_bug_release_times(mtvrptw_release_times):
    """
    This test exercises a bug that previously resulted in an incorrect time
    warp calculation caused by DurationSegment.finalise_front() not working
    correctly - it did not properly account for release times.
    """
    # This route visits 34, reloads, and then visits 23, 38, and 48, as
    # follows:
    # - Leave the depot at 14579.
    # - Visit 34 at 14902, leave at 15802.
    # - Return to reload depot at 16125.
    # - Visit 23 at 16430, leave at 17330.
    # - Visit 38 at 12230, adding 5395 time warp. Leave at 13130.
    # - Visit 48 at 28560, leave at 29460.
    # - Return to depot at 29567.
    route1 = Route(mtvrptw_release_times, 0, 0)
    for loc in [34, 0, 23, 38, 48]:
        route1.append(Node(loc=loc))
    route1.update()

    assert_equal(route1.distance(), 1713)
    assert_equal(route1.time_warp(), 5395)
    assert_equal(route1.duration(), 29567 + 5395 - 14579)

    # This route visits 6, as follows:
    # - Leave the depot at 4718.
    # - Visit 6 at 4970, leave at 5870.
    # - Return to depot at 6122.
    route2 = Route(mtvrptw_release_times, 1, 0)
    route2.append(Node(loc=6))
    route2.update()

    assert_equal(route2.distance(), 504)
    assert_equal(route2.time_warp(), 0)
    assert_equal(route2.duration(), 6122 - 4718)

    assert_equal(str(route1), "34 | 23 38 48")
    assert_equal(str(route2), "6")

    op = TripRelocate(mtvrptw_release_times)
    cost_eval = CostEvaluator([0], 1, 0)
    delta_cost = op.evaluate(route1[3], route2[1], cost_eval)
    assert_(delta_cost < 0)

    op.apply(route1[3], route2[1])
    assert_equal(str(route1), "34 | 38 48")
    assert_equal(str(route2), "6 | 23")

    route1.update()
    assert_equal(route1.distance(), 1525)
    assert_equal(route1.time_warp(), 2865)

    route2.update()
    assert_equal(route2.distance(), 1114)
    assert_(not route2.has_time_warp())

    delta_dist = 1525 + 1114 - 504 - 1713  # = new minus old
    delta_time_warp = 2865 - 5395  # = new minus old
    assert_equal(delta_cost, delta_dist + delta_time_warp)

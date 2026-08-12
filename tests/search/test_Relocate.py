import numpy as np
import pytest
from numpy.testing import assert_, assert_equal

from pyvrp import (
    Activity,
    Client,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    RandomNumberGenerator,
    Solution,
    VehicleType,
)
from pyvrp import Route as SolRoute
from pyvrp.search import (
    LocalSearch,
    NeighbourhoodParams,
    Relocate1,
    Relocate2,
    Relocate3,
    compute_neighbours,
)
from pyvrp.search._search import Node, Route
from tests.helpers import make_search_route


@pytest.mark.parametrize("operator", [Relocate1, Relocate2, Relocate3])
def test_relocate_uses_empty_routes(rc208, operator):
    """
    The relocate operators should be able to relocate clients to empty routes
    if that is an improvement.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(num_neighbours=rc208.num_clients)
    ls = LocalSearch(rc208, rng, compute_neighbours(rc208, nb_params))
    ls.add_operator(operator(rc208))

    single_route = list(range(rc208.num_clients))
    sol = Solution(rc208, [single_route])
    improved_sol = ls(sol, cost_evaluator, exhaustive=True)

    # The new solution should strictly improve on our original solution, and
    # should use more routes.
    assert_(improved_sol.num_routes() > 1)
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)


def test_cannot_relocate_when_parts_overlap_with_depot(ok_small):
    """
    RelocateN works by relocating N nodes starting at some node U to after some
    node V. But when there is no sequence of N nodes that does not contain the
    depot (because the routes are very short), then relocate is impossible.
    """
    cost_evaluator = CostEvaluator([20], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(num_neighbours=ok_small.num_clients)
    ls = LocalSearch(ok_small, rng, compute_neighbours(ok_small, nb_params))
    ls.add_operator(Relocate3(ok_small))

    sol = Solution(ok_small, [[0, 1], [2], [3]])
    new_sol = ls(sol, cost_evaluator, exhaustive=True)

    assert_equal(new_sol, sol)


def test_relocate_after_depot_should_work(ok_small):
    """
    This test exercises the bug identified in issue #142, involving a relocate
    action that should insert directly after the depot.
    """
    op = Relocate1(ok_small)

    # We create two routes: one with clients [C0, C1, C2], and the other empty.
    # It is an improving move to insert C2 into the empty route.
    route1 = Route(ok_small, vehicle_type=0)
    route2 = Route(ok_small, vehicle_type=0)

    nodes = [Node(descr) for descr in ["C0", "C1", "C2"]]
    for node in nodes:
        route1.append(node)
    route1.update()

    # This solution can be improved by moving C2 into its own route, that is,
    # inserting it after the depot of an empty route. Before the bug was fixed,
    # Relocate1 never performed this move.
    cost_evaluator = CostEvaluator([20], 6, 0)
    assert_(route1[3] is nodes[-1])
    cost, should_apply = op.evaluate(nodes[-1], route2[0], cost_evaluator)
    assert_(cost < 0)
    assert_(should_apply)

    assert_(nodes[-1].route is route1)
    assert_equal(route1.num_clients(), 3)
    assert_equal(route2.num_clients(), 0)

    # Apply the move and check that the routes and nodes are appropriately
    # updated.
    op.apply(nodes[-1], route2[0])
    route1.update()
    route2.update()
    assert_(nodes[-1].route is route2)
    assert_equal(route1.num_clients(), 2)
    assert_equal(route2.num_clients(), 1)


def test_relocate_only_happens_when_distance_and_duration_allow_it():
    """
    Tests that Relocate1 checks the duration matrix for time-window feasibility
    before applying a move that improves the travelled distance.
    """
    # Distance-wise, the best route is D0 -> C0 -> C1 -> D0. Duration-wise,
    # however, the best route is D0 -> C1 -> C0 -> D0.
    data = ProblemData(
        locations=[Location(0, 0), Location(1, 0), Location(2, 0)],
        clients=[
            Client(location=1, tw_early=0, tw_late=5),
            Client(location=2, tw_early=0, tw_late=5),
        ],
        depots=[Depot(location=0)],
        vehicle_types=[VehicleType(1, tw_early=0, tw_late=10)],
        distance_matrices=[
            np.asarray(
                [
                    [0, 1, 5],
                    [5, 0, 1],
                    [1, 5, 0],
                ]
            ),
        ],
        duration_matrices=[
            np.asarray(
                [
                    [0, 100, 2],
                    [1, 0, 100],
                    [100, 2, 0],
                ]
            ),
        ],
    )

    # We consider two solutions. The first is duration optimal, and overall the
    # only feasible solution. This solution can thus not be improved further.
    # We also consider a distance-optimal solution that is not feasible. Since
    # we have non-zero time warp penalty, this solution should be improved into
    # the duration optimal solution.
    duration_optimal = Solution(data, [[1, 0]])
    distance_optimal = Solution(data, [[0, 1]])

    assert_(distance_optimal.distance() < duration_optimal.distance())
    assert_(duration_optimal.time_warp() < distance_optimal.time_warp())

    cost_evaluator = CostEvaluator([], 1, 0)
    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_operator(Relocate1(data))

    assert_equal(ls(duration_optimal, cost_evaluator), duration_optimal)
    assert_equal(ls(distance_optimal, cost_evaluator), duration_optimal)


def test_relocate_to_heterogeneous_empty_route(ok_small):
    """
    This test asserts that a customer will be relocated to a non-empty route
    with a different capacity even if there is another empty route in between.
    """
    vehicle_types = [VehicleType(1, capacity=[cap]) for cap in [12, 5, 1, 3]]
    data = ok_small.replace(vehicle_types=vehicle_types)

    # Use a huge cost for load penalties to make other aspects irrelevant
    cost_evaluator = CostEvaluator([100_000], 6, 0)
    rng = RandomNumberGenerator(seed=42)

    # This is a non-empty neighbourhood (so LS does not complain), but the only
    # client moves allowed by it will not improve the initial solution created
    # below. So the only improvements Relocate1 can make must come from moving
    # clients behind the depot of a route.
    neighbours = {Activity(f"C{idx}"): [] for idx in range(data.num_clients)}
    neighbours[Activity("C1")].append(Activity("C0"))

    ls = LocalSearch(data, rng, neighbours)
    ls.add_operator(Relocate1(data))

    # Solution has routes with loads [13, 5, 0, 0] and excess [1, 0, 0, 0].
    # Moving C2 to route 4 will resolve all load penalties, but other moves
    # would increase load penalties. Therefore, this requires moving to an
    # empty route which is not the first.
    sol = Solution(
        data, [SolRoute(data, [0, 1, 2], 0), SolRoute(data, [3], 1)]
    )
    expected = Solution(
        data,
        [
            SolRoute(data, [0, 1], 0),
            SolRoute(data, [3], 1),
            SolRoute(data, [2], 3),
        ],
    )
    assert_equal(ls(sol, cost_evaluator), expected)


@pytest.mark.parametrize(
    ("op", "base_cost", "fixed_cost"),
    [
        (Relocate1, 2_346, 0),  # inexact; this move shortcuts
        (Relocate1, 2_346, 100),  # inexact; this move shortcuts
        (Relocate2, 1_417, 0),
        (Relocate2, 1_417, 9),
        (Relocate3, 135, 53),
        (Relocate3, 135, 997),
    ],
)
def test_relocate_fixed_vehicle_cost(ok_small, op, base_cost, fixed_cost):
    """
    Tests that relocate operators also take into account fixed vehicle costs
    changes if one of the routes is empty. In particular, we fix the base cost
    of evaluating the route changes (that's not changed), and vary the fixed
    vehicle cost. The total delta cost should also vary as a result.
    """
    vehicle_type = VehicleType(2, capacity=[10], fixed_cost=fixed_cost)
    data = ok_small.replace(vehicle_types=[vehicle_type])
    op = op(data)

    route1 = make_search_route(data, ["C1", "C3", "C0", "C2"])
    route2 = make_search_route(data, [])

    # First route is not empty, second route is. The operator evaluates moving
    # some nodes to the second route, which would use both of them. That should
    # add to the fixed vehicle cost.
    cost_eval = CostEvaluator([1], 1, 0)
    actual, should_apply = op.evaluate(route1[1], route2[0], cost_eval)
    assert_equal(actual, base_cost + fixed_cost)
    assert_(not should_apply)  # all worse


@pytest.mark.parametrize(("max_dur", "cost"), [(0, -4_044), (5_000, 956)])
def test_relocate_with_duration_constraint(ok_small, max_dur, cost):
    """
    Tests that the relocate operators correctly evaluate time warp due to
    maximum shift duration violations.
    """
    vehicle_type = VehicleType(2, capacity=[10], shift_duration=max_dur)
    data = ok_small.replace(vehicle_types=[vehicle_type])
    op = Relocate2(data)

    route1 = make_search_route(data, ["C1", "C3"])
    route2 = make_search_route(data, ["C0", "C2"])

    # Without duration constraint, route1 has a duration of 5_229 and no time
    # warp while route2 has a duration of 5_814 and timewarp 2_087, for a net
    # duration of 5_814 - 2_087 = 3_727 so no violation.
    # Consolidation into a single route may or may not be improving as the
    # total distance decreases but the maximum duration violation increases.
    # Moving from the first to the second route reduces the maximum duration
    # violation in the first route and is typically improving.
    assert_equal(route1.duration(), 5_229)
    assert_equal(route2.duration(), 5_814)

    cost_eval = CostEvaluator([1], 1, 0)
    delta, should_apply = op.evaluate(route1[1], route2[1], cost_eval)
    assert_equal(delta, cost)
    assert_equal(should_apply, cost < 0)


def test_within_route_simultaneous_pickup_and_delivery():
    """
    Tests that the Relocate operators correctly evaluate load violations within
    the same route.
    """
    data = ProblemData(
        locations=[
            Location(0, 0),
            Location(1, 0),
            Location(2, 0),
            Location(2, 0),
        ],
        clients=[
            Client(location=1, pickup=[5]),
            Client(location=2, pickup=[0]),
            Client(location=3, delivery=[5]),
        ],
        depots=[Depot(location=0)],
        vehicle_types=[VehicleType(capacity=[5])],
        distance_matrices=[np.where(np.eye(4), 0, 1)],
        duration_matrices=[np.zeros((4, 4), dtype=int)],
    )

    # Route is C0 -> C1 -> C2, and gets C0's pickup amount (5) before dropping
    # off C2's delivery amount (5). So total load is 10, and the excess load 5.
    route = make_search_route(data, ["C0", "C1", "C2"])
    assert_(not route.is_feasible())
    assert_equal(route.load(), [10])
    assert_equal(route.excess_load(), [5])

    # For Relocate1, we evaluate inserting C0 after C2. That resolves the
    # excess load.
    op = Relocate1(data)
    cost_eval = CostEvaluator([1], 1, 0)
    assert_equal(op.evaluate(route[1], route[3], cost_eval), (-5, True))


@pytest.mark.parametrize(
    ("max_distance", "expected"),
    [
        (5_000, -3_332),  # move reduces max_distance violation
        (2_500, -6_542),  # which becomes more important here
        (0, 18_458),  # but with only violations the move is not improving
    ],
)
def test_relocate_max_distance(ok_small, max_distance: int, expected: int):
    """
    Tests that a relocate move correctly evaluates maximum distance constraint
    violations, and can identify improving moves that increase overall distance
    but reduce the maximum distance violation.
    """
    vehicle_type = VehicleType(2, capacity=[10], max_distance=max_distance)
    data = ok_small.replace(vehicle_types=[vehicle_type])

    route1 = make_search_route(data, ["C0", "C1"])
    route2 = make_search_route(data, [])

    assert_equal(route1.distance(), 5_501)
    assert_equal(route1.excess_distance(), max(5_501 - max_distance, 0))

    cost_eval = CostEvaluator([0], 0, 10)
    op = Relocate1(data)

    # Moving C1 from route1 to route2 does not improve the overall distance,
    # but can be helpful in reducing maximum distance violations.
    actual, should_apply = op.evaluate(route1[2], route2[0], cost_eval)
    assert_equal(actual, expected)
    assert_equal(should_apply, expected < 0)
    op.apply(route1[2], route2[0])

    route1.update()
    assert_equal(route1.distance(), 3_270)
    assert_equal(route1.excess_distance(), max(3_270 - max_distance, 0))

    route2.update()
    assert_equal(route2.distance(), 3_909)
    assert_equal(route2.excess_distance(), max(3_909 - max_distance, 0))

    delta_dist = 3_270 + 3_909 - 5_501  # compare manual delta cost
    delta_excess = sum(
        [
            max(3_270 - max_distance, 0),
            max(3_909 - max_distance, 0),
            -max(5_501 - max_distance, 0),
        ]
    )
    assert_equal(delta_dist + 10 * delta_excess, expected)


@pytest.mark.parametrize("instance", ["ok_small", "pr107", "prize_collecting"])
def test_supports_clients(instance, request):
    """
    Tests that Relocate operators support any type of data instance with
    regular clients.
    """
    data = request.getfixturevalue(instance)
    assert_(Relocate1.supports(data))


def test_supports_shipments(small_shipments):
    """
    Tests that only the even Relocate operators support instances with pure
    shipments.
    """
    # This is an instance with pure shipments - there are no clients.
    assert_equal(small_shipments.num_clients, 0)
    assert_equal(small_shipments.num_shipments, 4)

    # These move an even number of nodes between routes, and thus support
    # instances with pure shipments.
    assert_(Relocate2.supports(small_shipments))

    # But these operators move an odd number of nodes between routes, and that
    # is not supported.
    assert_(not Relocate1.supports(small_shipments))


def test_bug_release_time_shift_time_windows():
    """
    Tests that a bug involving release times and restricted vehicle shifts has
    been fixed. See #852 for details.
    """
    data = ProblemData(
        locations=[Location(0, 0)],
        clients=[
            Client(location=0, tw_early=2, release_time=2),
            Client(location=0, tw_early=2, release_time=2),
        ],
        depots=[Depot(location=0)],
        vehicle_types=[VehicleType(), VehicleType(tw_late=1)],
        distance_matrices=[np.zeros((1, 1), dtype=int)],
        duration_matrices=[np.zeros((1, 1), dtype=int)],
    )

    route1 = make_search_route(data, ["C0"], vehicle_type=0)
    assert_(route1.is_feasible())

    # Vehicle's time windows are constrained to [0, 1], but C1 is not released
    # until 2. So there's a unit of time warp when we leave the depot. Then we
    # have to wait until 2 at the client, and have another unit of time warp
    # when we return to the depot.
    route2 = make_search_route(data, ["C1"], vehicle_type=1)
    assert_equal(route2.time_warp(), 2)

    # This move proposes inserting C0 before C2 in route2. That changes nothing
    # about route2's time warp, so the move should not affect costs.
    op = Relocate1(data)
    cost_eval = CostEvaluator([], 1, 0)
    assert_equal(op.evaluate(route1[1], route2[0], cost_eval), (0, False))


def test_empty_route_delta_cost_bug():
    """
    Tests that a bug identified in #853 has been fixed. The bug caused empty
    routes' costs to be incorrectly included in delta cost evaluations.
    """
    mat = [
        [0, 5, 0],
        [5, 0, 0],
        [0, 0, 0],
    ]
    # Empty route with vehicle type 0 has no cost, but an empty route with
    # vehicle type 1 has cost 10 (5 distance, 5 time warp).
    vehicle_types = [
        VehicleType(1),
        VehicleType(1, start_depot=0, end_depot=1, shift_duration=0),
    ]
    data = ProblemData(
        locations=[Location(0, 0), Location(0, 0), Location(0, 0)],
        depots=[Depot(location=0), Depot(location=1)],
        clients=[Client(location=2)],
        vehicle_types=vehicle_types,
        duration_matrices=[mat],
        distance_matrices=[mat],
    )

    route1 = make_search_route(data, ["C0"], vehicle_type=0)
    route2 = make_search_route(data, [], vehicle_type=1)

    # This move proposes inserting C0 in route2. Before fixing the bug,
    # route2's cost was included in the delta cost computation, claiming
    # this to be an improving move. But an empty route's cost should not be
    # included in the delta cost.
    op = Relocate1(data)
    cost_eval = CostEvaluator([], 1, 1)
    assert_equal(op.evaluate(route1[1], route2[0], cost_eval), (0, False))


def test_relocate_overtime(ok_small_overtime):
    """
    Tests a relocate move involving overtime correctly evaluates the resulting
    (duration-based) cost delta.
    """
    route1 = make_search_route(ok_small_overtime, ["C0", "C2"])
    route2 = make_search_route(ok_small_overtime, [])

    # First route takes 5'814, of which 814 is overtime. The cost structure
    # is 1x duration + 10x overtime.
    assert_equal(route1.duration(), 5_814)
    assert_equal(route1.overtime(), 814)
    old_cost = 5_814 + 10 * 814

    # The move evaluates the new routes [C0] and [C2]. Those have the following
    # durations (travel to client, service, travel back to depot):
    # - route 1: 1544 + 360 + 1726 = 3630,
    # - route 2: 1931 + 420 + 2063 = 4414.
    # Neither route has overtime, so these routes only have duration costs.
    new_cost = 3630 + 4414

    op = Relocate1(ok_small_overtime)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(
        op.evaluate(route1[2], route2[0], cost_eval),
        (new_cost - old_cost, True),
    )


def test_skip_unassigned_clients(ok_small):
    """
    Tests that the operators do not evaluate moves for unassigned clients.
    """
    route = make_search_route(ok_small, ["C0", "C1"])
    node = Node("C2")  # unassigned

    operator = Relocate1(ok_small)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(operator.evaluate(node, route[0], cost_eval), (0, False))


def test_name(ok_small):
    """
    Tests accessing the operator's name attribute.
    """
    assert_equal(Relocate1(ok_small).name, "Relocate1")
    assert_equal(Relocate3(ok_small).name, "Relocate3")


def test_relocate_shipment(small_shipments):
    """
    Tests that the relocate operators can also move shipments.
    """
    activities = ["L1", "U1", "L0", "U0", "L2", "U2", "L3", "U3"]
    route = make_search_route(small_shipments, activities)
    assert_equal(route.distance(), 64_267)

    op = Relocate2(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)

    # These moves cannot be done because they would move part of a shipment,
    # possibly resulting in a pickup after a delivery.
    assert_equal(op.evaluate(route[2], route[0], cost_eval), (0, False))
    assert_equal(op.evaluate(route[4], route[0], cost_eval), (0, False))
    assert_equal(op.evaluate(route[6], route[0], cost_eval), (0, False))

    # But those one can: moving L3 U3 to the front of the route is perfectly
    # fine, and an improving move.
    assert_equal(op.evaluate(route[7], route[0], cost_eval), (-13_838, True))
    op.apply(route[7], route[0])
    route.update()

    assert_equal(route.distance(), 50_429)
    assert_equal(str(route), "L3 U3 L1 U1 L0 U0 L2 U2")


def test_relocate_shipment_fixed_cost(small_shipments):
    """
    Tests that relocating a shipment accounts for fixed cost if it leaves the
    route empty.
    """
    veh_type = small_shipments.vehicle_type(0).replace(fixed_cost=10_000)
    data = small_shipments.replace(vehicle_types=[veh_type])

    route1 = make_search_route(data, ["L0", "U0"])
    route2 = make_search_route(data, ["L3", "U3"])
    assert_equal(route1.distance() + route2.distance(), 29_265)

    # Move results in 1_902 less distance, but also empties route1, which saves
    # a fixed cost of 10_000. So delta is 11_902.
    op = Relocate2(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route1[1], route2[2], cost_eval), (-11_902, True))

    op.apply(route1[1], route2[2])
    route1.update()
    route2.update()

    # route1 is now empty, and route2 has 1_902 less distance than the two
    # routes had previously.
    assert_equal(route1.distance(), 0)
    assert_equal(route2.distance(), 29_265 - 1_902)
    assert_equal(str(route1), "")
    assert_equal(str(route2), "L3 U3 L0 U0")

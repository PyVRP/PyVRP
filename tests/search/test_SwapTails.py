import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal
from pytest import mark

import pyvrp
from pyvrp import (
    Activity,
    Client,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    RandomNumberGenerator,
    VehicleType,
)
from pyvrp import Route as SolRoute
from pyvrp.search import LocalSearch, SwapTails
from pyvrp.search._search import Node, Solution


def make_search_solution(data: ProblemData, routes: list[SolRoute]):
    """
    Creates a pyvrp.search.Solution from the given routes. We need this because
    SwapTails compares pointers to routes, which assumes a particular memory
    layout that Python does not normally respect. Laying out the routes inside
    a pyvrp.search.Solution does.
    """
    sol = Solution(data)
    sol.load(pyvrp.Solution(data, routes))
    return sol


@mark.parametrize(
    "vehicle_types",
    [
        [VehicleType(capacity=[10]), VehicleType(capacity=[10])],
        [VehicleType(capacity=[8]), VehicleType(capacity=[10])],
        [VehicleType(capacity=[10]), VehicleType(capacity=[8])],
        [VehicleType(capacity=[9]), VehicleType(capacity=[9])],
        [VehicleType(capacity=[8]), VehicleType(capacity=[8])],
    ],
)
def test_OkSmall_multiple_vehicle_types(
    ok_small, vehicle_types: list[VehicleType]
):
    """
    This test evaluates a move that is improving for some of the vehicle types,
    and not for others. Because the granular neighbourhood is very constrained,
    only a single move can be applied. When it's better to do so, it should
    have been applied, and when it's not better, it should not.
    """
    data = ok_small.replace(vehicle_types=vehicle_types)

    cost_evaluator = CostEvaluator([10_000], 6, 0)  # large load penalty
    rng = RandomNumberGenerator(seed=42)

    neighbours: dict[Activity, list[Activity]] = {
        Activity(f"C{idx}"): [] for idx in range(data.num_clients)
    }
    neighbours[Activity("C0")] = [Activity("C1")]  # only C0 -> C1

    ls = LocalSearch(data, rng, neighbours)
    ls.add_operator(SwapTails(data))

    routes1 = [SolRoute(data, [0, 2], 0), SolRoute(data, [1, 3], 1)]
    sol1 = pyvrp.Solution(data, routes1)

    routes2 = [SolRoute(data, [0, 3], 0), SolRoute(data, [1, 2], 1)]
    sol2 = pyvrp.Solution(data, routes2)

    cost1 = cost_evaluator.penalised_cost(sol1)
    cost2 = cost_evaluator.penalised_cost(sol2)
    assert_(not np.allclose(cost1, cost2))

    # Using the local search, the result should not get worse.
    improved_sol1 = ls(sol1, cost_evaluator, exhaustive=True)
    improved_sol2 = ls(sol2, cost_evaluator, exhaustive=True)

    expected_sol = sol1 if cost1 < cost2 else sol2
    assert_equal(improved_sol1, expected_sol)
    assert_equal(improved_sol2, expected_sol)


def test_move_involving_empty_routes():
    """
    This test checks that a SwapTails move that turns an empty route into a
    non-empty route (or vice-versa) is correctly evaluated.
    """
    data = ProblemData(
        locations=[Location(0, 0), Location(1, 1), Location(1, 0)],
        clients=[Client(1), Client(2)],
        depots=[Depot(0)],
        vehicle_types=[
            VehicleType(fixed_cost=10),
            VehicleType(fixed_cost=100),
        ],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    # First route is [C0, C1], second route is empty.
    sol = make_search_solution(data, [SolRoute(data, [0, 1], 0)])
    route1, route2 = sol.routes

    op = SwapTails(data)
    cost_eval = CostEvaluator([], 0, 0)

    # This move does not change the route structure, so the delta cost is 0.
    assert_allclose(op.evaluate(route1[2], route2[0], cost_eval), (0, False))

    # This move creates routes (D0 -> C0 -> D0) and (D0 -> C1 -> D0), making
    # route 2 non-empty and thus incurring its fixed cost of 100.
    assert_allclose(op.evaluate(route1[1], route2[0], cost_eval), (100, False))

    # Now we reverse the visits of route 1 and 2, so that we can hit the cases
    # where route 1 is empty.
    route1.clear()

    for activity in ["C0", "C1"]:
        route2.append(Node(activity))

    route1.update()  # D0 -> D0
    route2.update()  # D0 -> C0 -> C1 -> D0

    # This move does not change the route structure, so the delta cost is 0.
    assert_allclose(op.evaluate(route2[2], route1[0], cost_eval), (0, False))

    # This move creates routes (D0 -> C1 -> D0) and (D0 -> C0 -> D0), making
    # route 1 non-empty and thus incurring its fixed cost of 10.
    assert_allclose(op.evaluate(route2[1], route1[0], cost_eval), (10, False))


def test_move_involving_multiple_depots():
    """
    Tests that SwapTails correctly evaluates distance changes due to different
    start and end depots of different vehicle types.
    """
    # Locations with indices 0 and 1 are depots, with 2 and 3 are clients.
    data = ProblemData(
        locations=[
            Location(0, 0),
            Location(5, 5),
            Location(1, 1),
            Location(4, 4),
        ],
        clients=[Client(2), Client(3)],
        depots=[Depot(0), Depot(1)],
        vehicle_types=[
            VehicleType(start_depot=0, end_depot=0),
            VehicleType(start_depot=1, end_depot=1),
        ],
        distance_matrices=[
            [
                [0, 10, 2, 8],
                [10, 0, 8, 2],
                [2, 8, 0, 6],
                [8, 2, 6, 0],
            ]
        ],
        duration_matrices=[np.zeros((4, 4), dtype=int)],
    )

    routes = [SolRoute(data, [1], 0), SolRoute(data, [0], 1)]
    sol = make_search_solution(data, routes)
    route1, route2 = sol.routes

    assert_equal(route1.distance(), 16)
    assert_equal(route2.distance(), 16)

    op = SwapTails(data)
    cost_eval = CostEvaluator([], 1, 0)

    # This is a no-op, and should be ignored.
    assert_allclose(op.evaluate(route1[1], route2[1], cost_eval), (0, False))

    # First would be D0 -> C1 -> C0 -> D0, second D1 -> D1. Distance on route2
    # would be zero, and on route1 16. Thus delta cost is -16.
    assert_allclose(op.evaluate(route1[1], route2[0], cost_eval), (-16, True))


def test_move_with_different_profiles(ok_small_two_profiles):
    """
    Tests that SwapTails correctly evaluates moves between routes with
    different profiles.
    """
    data = ok_small_two_profiles
    dist1, dist2 = data.distance_matrices()

    routes = [SolRoute(data, [2], 0), SolRoute(data, [1], 1)]
    sol = make_search_solution(data, routes)
    route1, route2 = sol.routes[0], sol.routes[3]

    op = SwapTails(data)
    cost_eval = CostEvaluator([0], 0, 0)  # all zero so no costs from penalties

    # First route has profile 0, and its distance is thus computed using the
    # first distance matrix.
    assert_equal(route1.profile(), 0)
    assert_equal(route1.distance(), dist1[0, 3] + dist1[3, 0])

    # Second route has profile 1, and its distance is thus computed using the
    # second distance matrix.
    assert_equal(route2.profile(), 1)
    assert_equal(route2.distance(), dist2[0, 2] + dist2[2, 0])

    # This move evaluates the setting where the second route would be empty,
    # and the first becomes D0 -> C2 -> C1 -> D0.
    delta = dist1[3, 2] + dist1[2, 0] - dist1[3, 0] - route2.distance()
    assert_allclose(
        op.evaluate(route1[1], route2[0], cost_eval), (delta, True)
    )


def test_supports(ok_small, pr107, small_shipments):
    """
    Tests that SwapTails does not support TSP or pure shipment instances.
    """
    assert_(SwapTails.supports(ok_small))  # is a regular VRP
    assert_(not SwapTails.supports(pr107))  # is a TSP
    assert_(not SwapTails.supports(small_shipments))  # is pure shipment


def test_skips_move_if_shipment_is_split_between_routes(small_shipments):
    """
    Tests that SwapTails skips a move if it would leave part of a shipment
    in another route.
    """
    data = small_shipments.replace(clients=[Client(0, delivery=[0])])
    assert_(SwapTails.supports(data))

    route1 = SolRoute(data, [Activity("L0"), Activity("U0")], 0)
    route2 = SolRoute(data, [Activity("L1"), Activity("U1")], 0)
    sol = make_search_solution(data, [route1, route2])

    pickup1, _ = sol.shipments[0]
    _, delivery2 = sol.shipments[1]

    op = SwapTails(data)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_allclose(op.evaluate(pickup1, delivery2, cost_eval), (0, False))


def _electric_solution(data, routes: list[list[str]]) -> Solution:
    """
    Loads the given routes of activities into a search solution, where the
    route at each index uses the vehicle type of that index.
    """
    return make_search_solution(
        data,
        [
            SolRoute(data, list(map(Activity, visits)), idx)
            for idx, visits in enumerate(routes)
            if visits
        ],
    )


def test_moves_chargers_in_tails(small_electric):
    """
    Tests that chargers in the tails move along to the other route.
    """
    veh_type = small_electric.vehicle_type(0).replace(num_available=1)
    data = small_electric.replace(vehicle_types=[veh_type, veh_type])
    sol = _electric_solution(data, [["C0", "C1", "R1"], ["C2", "R0"]])

    route1, route2 = sol.routes
    op = SwapTails(data)
    cost_eval = CostEvaluator([], 0, 0)

    # Before: distance 100 and 80. After: distance 40 and 120.
    assert_allclose(op.evaluate(route1[1], route2[1], cost_eval), (-20, True))

    op.apply(route1[1], route2[1])
    route1.update()
    route2.update()

    assert_equal(str(route1), "C0 R0")
    assert_equal(str(route2), "C2 C1 R1")
    assert_equal(route1.num_chargers(), 1)
    assert_equal(route2.num_chargers(), 1)
    assert_equal(route1.distance() + route2.distance(), 160)


def test_tail_battery_uses_vehicle_of_new_route(small_electric):
    """
    Tests that a tail moved to another route is evaluated using the battery
    capacity of the vehicle of that route.
    """
    large = small_electric.vehicle_type(0).replace(
        num_available=1, battery_capacity=100
    )
    small = large.replace(battery_capacity=50)
    data = small_electric.replace(vehicle_types=[large, small])
    sol = _electric_solution(data, [["C0", "R0", "C1", "C2"], []])

    route1, route2 = sol.routes
    assert_equal(route1.excess_charge(), 0)

    op = SwapTails(data)
    cost_eval = CostEvaluator([], 0, 1)

    # The tail moves to the empty route of the vehicle with the smaller battery
    # where its last stretch of 60 exceeds the capacity by 10. The new routes
    # have distance 20 and 80, compared to the 80 of the current route.
    delta_cost, _ = op.evaluate(route1[1], route2[0], cost_eval)
    assert_allclose(delta_cost, 20 + 80 + 10 - 80)


def test_skips_chargers_if_recharge_duration_differs(small_electric):
    """
    Tests that the move is not evaluated if the tails contain chargers, and the
    vehicles have different recharge durations.
    """
    veh_type1 = small_electric.vehicle_type(0).replace(
        num_available=1, recharge_duration=5
    )
    veh_type2 = veh_type1.replace(recharge_duration=10)
    data = small_electric.replace(vehicle_types=[veh_type1, veh_type2])
    sol = _electric_solution(data, [["C0", "C1", "R1"], ["C2"]])

    route1, route2 = sol.routes
    op = SwapTails(data)
    cost_eval = CostEvaluator([], 0, 0)

    assert_allclose(op.evaluate(route1[1], route2[1], cost_eval), (0, False))


def test_evaluates_tails_without_charger_if_recharge_duration_differs(
    small_electric,
):
    """
    Tests that a different recharge duration is no problem if the tails do not
    contain a charger.
    """
    veh_type1 = small_electric.vehicle_type(0).replace(
        num_available=1, recharge_duration=5
    )
    veh_type2 = veh_type1.replace(recharge_duration=10)
    data = small_electric.replace(vehicle_types=[veh_type1, veh_type2])
    sol = _electric_solution(data, [["R1", "C0", "C1"], ["C2"]])

    route1, route2 = sol.routes
    op = SwapTails(data)
    cost_eval = CostEvaluator([], 0, 0)

    # Before: distance 140 and 80. After: distance 100 and 80.
    assert_allclose(op.evaluate(route1[2], route2[1], cost_eval), (-40, True))


def test_does_not_create_route_with_only_chargers(small_electric):
    """
    Tests that the move is skipped when it would leave a route with only
    chargers, because such a route is empty.
    """
    veh_type = small_electric.vehicle_type(0).replace(num_available=1)
    data = small_electric.replace(vehicle_types=[veh_type, veh_type])
    sol = _electric_solution(data, [["C0", "R0"], []])

    route1, route2 = sol.routes
    op = SwapTails(data)
    cost_eval = CostEvaluator([], 0, 0)

    assert_allclose(op.evaluate(route1[1], route2[0], cost_eval), (0, False))


def test_empties_route_with_only_charger_before(small_electric):
    """
    Tests that moving all clients out of a route counts as emptying the route,
    also if a charger is left behind.
    """
    veh_type = small_electric.vehicle_type(0).replace(num_available=1)
    data = small_electric.replace(vehicle_types=[veh_type, veh_type])
    sol = _electric_solution(data, [["C0"], ["R0", "C1"]])

    route1, route2 = sol.routes
    op = SwapTails(data)
    cost_eval = CostEvaluator([], 0, 0)

    # Before: distance 20 and 60. After: distance 60, and an empty route.
    assert_allclose(op.evaluate(route1[1], route2[1], cost_eval), (-20, True))

    op.apply(route1[1], route2[1])
    route1.update()
    route2.update()

    assert_equal(str(route1), "C0 C1")
    assert_equal(route2.num_clients(), 0)

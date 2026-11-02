import numpy as np
import pytest
from numpy.testing import assert_, assert_allclose, assert_equal

import pyvrp
from pyvrp import (
    Activity,
    Charger,
    Client,
    ClientGroup,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    Shipment,
    VehicleType,
)
from pyvrp.search import (
    OPERATORS,
    BinaryOperator,
    InsertCharger,
    InsertOptionalClient,
    RelocateShipment,
    RelocateWithDepot,
    RemoveAdjacentDepot,
)
from pyvrp.search._search import Solution
from pyvrp.stop import MaxIterations
from tests.helpers import make_search_route


def _line_data(capacity: int = 2) -> ProblemData:
    """
    Creates an instance where all locations are on a line. The distance and
    duration between two locations is the difference in their coordinates:

        depot: 0, C0: 10, L0: 20, R0: 25, C1: 30, U0: 40, C2: 45.

    C2 is optional, with a prize of 100. There are two vehicles that can reload
    once, each with the given capacity and a battery capacity of 50.
    """
    coords = [0, 10, 30, 45, 20, 40, 25]
    matrix = np.abs(np.subtract.outer(coords, coords))

    return ProblemData(
        locations=[Location(x, 0) for x in coords],
        clients=[
            Client(1, delivery=[1]),
            Client(2, delivery=[1]),
            Client(3, delivery=[0], required=False, prize=100),
        ],
        depots=[Depot(0)],
        vehicle_types=[
            VehicleType(
                2,
                capacity=[capacity],
                battery_capacity=50,
                recharge_duration=5,
                reload_depots=[0],
                max_reloads=1,
            ),
        ],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
        shipments=[Shipment(4, 5, amount=[1])],
        chargers=[Charger(6)],
    )


def test_insert_charger_does_not_change_load():
    """
    Tests that InsertCharger only changes the battery on a route with a load
    violation, because visiting a charger does not change the load.
    """
    data = _line_data(capacity=1)
    route = make_search_route(data, ["C0", "L0", "U0", "C1"])

    # The route has distance 80, and thus excess charge 30.
    assert_equal(route.excess_charge(), 30)
    excess_load = route.excess_load()
    assert_equal(excess_load, [1])

    # R0 between L0 and U0 is on the way, so the distance does not change. The
    # vehicle then drains 25 up to R0 and 55 after it, which leaves excess
    # charge 5. That saves 25 units of charge, at a penalty of 10 each.
    op = InsertCharger(data)
    cost_eval = CostEvaluator([1000], 0, 10)
    assert_allclose(op.evaluate(route[2], cost_eval), (-250, True))

    op.apply(route[2])
    route.update()

    assert_equal(str(route), "C0 L0 R0 U0 C1")
    assert_equal(route.distance(), 80)
    assert_equal(route.excess_charge(), 5)
    assert_equal(route.excess_load(), excess_load)


def test_relocate_shipment_changes_excess_charge_of_both_routes():
    """
    Tests that RelocateShipment accounts for the charge used by both routes.
    """
    data = _line_data()
    sol = Solution(data)
    route1 = make_search_route(data, [sol.clients[1], *sol.shipments[0]])
    route2 = make_search_route(data, [sol.clients[0]])

    assert_equal(route1.distance(), 100)
    assert_equal(route1.excess_charge(), 50)
    assert_equal(route2.distance(), 20)
    assert_equal(route2.excess_charge(), 0)

    # After the move, the routes have distance 60 and 80 and excess charge 10
    # and 30. The total distance thus increases by 20, but the excess charge
    # decreases by 10, at a penalty of 10 for each unit.
    op = RelocateShipment(data)
    cost_eval = CostEvaluator([0], 0, 10)
    assert_allclose(op.evaluate(route1[2], route2[1], cost_eval), (-80, True))

    op.apply(route1[2], route2[1])
    route1.update()
    route2.update()

    assert_equal(str(route1), "C1")
    assert_equal(str(route2), "C0 L0 U0")
    assert_equal(route1.excess_charge(), 10)
    assert_equal(route2.excess_charge(), 30)


def test_relocate_with_depot_does_not_recharge():
    """
    Tests that RelocateWithDepot does not treat the reload depot as a charger.
    """
    data = _line_data(capacity=1)
    route = make_search_route(data, ["C0", "C1"])

    assert_equal(route.distance(), 60)
    assert_equal(route.excess_charge(), 10)
    assert_equal(route.excess_load(), [1])

    # Visiting C1 first and then reloading resolves the excess load, at 20 more
    # distance. Because the depot does not recharge the vehicle, that distance
    # is also 20 more excess charge.
    op = RelocateWithDepot(data)
    cost_eval = CostEvaluator([1000], 0, 1)
    assert_allclose(
        op.evaluate(route[1], route[2], cost_eval),
        (20 + 20 - 1000, True),
    )

    op.apply(route[1], route[2])
    route.update()

    assert_equal(str(route), "C1 | C0")
    assert_equal(route.distance(), 80)
    assert_equal(route.excess_charge(), 30)
    assert_equal(route.excess_load(), [0])


def test_remove_adjacent_depot_saves_charge():
    """
    Tests that RemoveAdjacentDepot accounts for the charge that is saved by not
    visiting the reload depot. That depot does not recharge the vehicle.
    """
    data = _line_data()
    route = make_search_route(data, ["C0", "D0", "C1"])

    assert_equal(route.distance(), 80)
    assert_equal(route.excess_charge(), 30)

    # The route without D0 has distance 60 and excess charge 10.
    op = RemoveAdjacentDepot(data)
    cost_eval = CostEvaluator([0], 0, 10)
    assert_allclose(op.evaluate(route[1], cost_eval), (-220, True))

    op.apply(route[1])
    route.update()

    assert_equal(str(route), "C0 C1")
    assert_equal(route.excess_charge(), 10)


def test_insert_optional_client_pays_for_excess_charge():
    """
    Tests that InsertOptionalClient weighs the prize of an optional client
    against the additional excess charge from visiting it.
    """
    data = _line_data()
    sol = Solution(data)
    route = make_search_route(data, ["C1"])
    client = sol.clients[2]

    # Visiting C2 gives a prize of 100, at an additional distance of 30.
    op = InsertOptionalClient(data)
    assert_allclose(
        op.evaluate(client, route[1], CostEvaluator([0], 0, 0)),
        (-70, True),
    )

    # The excess charge goes from 10 to 40, which is too expensive here.
    delta_cost, should_apply = op.evaluate(
        client, route[1], CostEvaluator([0], 0, 10)
    )
    assert_allclose(delta_cost, 230)
    assert_(not should_apply)


def _make_data(seed: int) -> ProblemData:
    """
    Creates a small instance that mixes batteries with capacities, time
    windows, reload depots, shipments, optional clients, and a client group.
    """
    rng = np.random.default_rng(seed)
    coords = rng.integers(0, 100, size=(17, 2))

    diff = coords[:, np.newaxis] - coords
    matrix = np.rint(np.hypot(*np.moveaxis(diff, 2, 0))).astype(int)

    clients = [
        Client(1, delivery=[2]),
        Client(2, delivery=[2]),
        Client(3, delivery=[1], tw_late=150),
        Client(4, delivery=[1], required=False, prize=200),
        Client(5, delivery=[1], required=False, group=0),
        Client(6, delivery=[1], required=False, group=0),
        Client(7, delivery=[1], required=False, prize=300),
    ]
    shipments = [
        Shipment(8, 9, amount=[2]),
        Shipment(10, 11, amount=[2], required=False, prize=200),
        Shipment(12, 13, amount=[2], required=False, prize=300),
    ]
    chargers = [Charger(14), Charger(15), Charger(16)]

    # Two vehicle types, because SwapTails then moves chargers between routes
    # that have a different battery and a different recharge duration.
    vehicle_types = [
        VehicleType(
            2,
            capacity=[5],
            battery_capacity=150,
            recharge_duration=10,
            reload_depots=[0],
            max_reloads=2,
        ),
        VehicleType(
            1,
            capacity=[4],
            battery_capacity=120,
            initial_charge=90,
            recharge_duration=10,
        ),
    ]

    return ProblemData(
        locations=[Location(int(x), int(y)) for x, y in coords],
        clients=clients,
        depots=[Depot(0)],
        vehicle_types=vehicle_types,
        distance_matrices=[matrix],
        duration_matrices=[matrix],
        groups=[ClientGroup([4, 5])],
        shipments=shipments,
        chargers=chargers,
    )


def _make_solution(data: ProblemData, seed: int) -> pyvrp.Solution:
    """
    Creates a random solution that visits chargers and reload depots, so that
    all operators have something to work with.
    """
    rng = np.random.default_rng(seed)

    # All required stops, one client of the group, and some of the optional
    # ones. Optional stops that are left out can then be inserted by the
    # operators that do that.
    stops = ["C0", "C1", "C2", "S0", f"C{rng.integers(4, 6)}"]
    optional = ["C3", "C6", "S1", "S2"]
    stops += [stop for stop in optional if rng.random() < 0.5]

    routes: list[list[str]] = [[] for _ in range(3)]
    for stop in rng.permutation(stops):
        activities = routes[rng.integers(3)]
        if stop.startswith("S"):  # pickup somewhere before the delivery
            first, second = np.sort(rng.integers(len(activities) + 1, size=2))
            activities.insert(second, f"U{stop[1:]}")
            activities.insert(first, f"L{stop[1:]}")
        else:
            activities.insert(rng.integers(len(activities) + 1), str(stop))

    for idx, activities in enumerate(routes):
        if not activities:
            continue

        for _ in range(rng.integers(3)):
            charger = rng.integers(data.num_chargers)
            activities.insert(rng.integers(len(activities) + 1), f"R{charger}")

        if idx < 2 and rng.random() < 0.5:  # only these vehicles can reload
            # Reloading is only allowed when no shipment is on board.
            positions = [
                pos
                for pos in range(1, len(activities))
                if sum(a[0] == "L" for a in activities[:pos])
                == sum(a[0] == "U" for a in activities[:pos])
            ]
            if positions:
                activities.insert(rng.choice(positions), "D0")

    return pyvrp.Solution(
        data,
        [
            pyvrp.Route(data, list(map(Activity, activities)), vehicle_type)
            for vehicle_type, activities in zip([0, 0, 1], routes)
            if activities
        ],
    )


def _load(op, sol: Solution, start: pyvrp.Solution) -> tuple[list, list]:
    """
    Loads the given solution, and returns the U and V nodes to evaluate. These
    are the nodes that ``LocalSearch`` would pass to the given operator.
    """
    sol.load(start)
    if hasattr(op, "init"):
        op.init(sol)

    us = [*sol.clients, *(node for pair in sol.shipments for node in pair)]
    if not isinstance(op, BinaryOperator):
        return us, [None]

    vs = []
    for V in us:
        if V.route is None:
            continue

        vs.append(V)
        prev = V.route[V.pos - 1]
        if prev.is_start_depot() or prev.is_charger():
            vs.append(prev)

    vs.extend(route[0] for route in sol.routes if len(route) == 2)
    return us, vs


@pytest.mark.parametrize("op_cls", OPERATORS)
def test_applied_moves_change_cost_as_predicted(op_cls):
    """
    Tests that each operator's predicted delta cost is exactly the change in
    the cost of the solution after applying the move, on battery routes that
    also have loads, shipments, reload depots and optional clients. The cost
    afterwards is calculated by ``pyvrp.Solution`` and thus does not depend on
    the concatenation code that is used to evaluate moves.
    """
    cost_eval = CostEvaluator([20], 5, 7)
    num_applied = 0

    for seed in range(40):
        data = _make_data(seed)
        assert_(op_cls.supports(data))

        op = op_cls(data)
        sol = Solution(data)
        start = _make_solution(data, seed)
        start_cost = cost_eval.penalised_cost(start)
        us, vs = _load(op, sol, start)

        for idx in range(len(us)):
            for jdx in range(len(vs)):
                args = [us[idx]] if vs[jdx] is None else [us[idx], vs[jdx]]
                if args[0] is args[-1] and len(args) == 2:
                    continue

                delta_cost, should_apply = op.evaluate(*args, cost_eval)
                if not should_apply:
                    continue

                op.apply(*args)
                for route in sol.routes:
                    route.update()

                cost = cost_eval.penalised_cost(sol.unload())
                assert_allclose(cost - start_cost, delta_cost)
                num_applied += 1

                us, vs = _load(op, sol, start)

    assert_(num_applied > 0)


@pytest.mark.parametrize("seed", range(1, 7))
def test_solves_instances_mixing_features(seed: int):
    """
    Tests the full search on instances that mix batteries with other features.
    The debug build checks all delta costs during this search. All clients are
    within range of the chargers for these seeds, unlike for seed 0, where one
    client is at least 79 away from all chargers on a battery of 150.
    """
    data = _make_data(seed)
    res = pyvrp.solve(data, MaxIterations(200), seed=seed, display=False)

    assert_(res.is_feasible(), msg=str(res.best))
    assert_equal(res.best.excess_distance(), 0)

"""
Regressions for false improvements and cycling with floating-point measures.

Each reproducer evaluates and applies a closed, two-move cycle explicitly.
This exposes the acceptance bug without risking a LocalSearch hang or a debug
assertion abort. Operators are recreated between moves to rule out stale
operator caches. Apart from Unix timestamps, all explicitly supplied numerical
inputs and computed route costs in these examples are below 1e9.
"""

import numpy as np
import pytest
from numpy.testing import assert_allclose, assert_equal

from pyvrp import (
    Client,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    Shipment,
    Solution,
    VehicleType,
)
from pyvrp.constants import TOL
from pyvrp.search import (
    InsertOptionalClient,
    Relocate1,
    Relocate2,
    RemoveOptionalClient,
)
from pyvrp.search._search import Node
from pyvrp.search._search import Solution as SearchSolution
from tests.helpers import make_search_route


def _route_cost(route, cost_eval):
    """The penalised cost of a nonempty search route, before unloading it."""
    return (
        route.fixed_vehicle_cost()
        + route.distance_cost()
        + route.duration_cost()
        + cost_eval.tw_penalty(route.time_warp())
        + cost_eval.dist_penalty(route.excess_distance(), 0)
        + sum(
            cost_eval.load_penalty(excess, 0, dim)
            for dim, excess in enumerate(route.excess_load())
        )
    )


def _assert_progress(evaluations):
    """A closed cycle cannot consist entirely of improving moves."""
    assert not all(apply for _, apply, _ in evaluations), (
        "Both directions of a closed cycle were accepted. "
        f"(predicted delta, should_apply, actual delta): {evaluations}"
    )

    for delta, should_apply, actual in evaluations:
        if should_apply:
            assert actual < -1e-6, (delta, actual)
            assert_allclose(delta, actual, rtol=0, atol=1e-6)


def _check_relocation_cycle(data, route, cost_eval, moves, operator=Relocate1):
    original = str(route)
    evaluations = []
    for u, v in moves:
        op = operator(data)
        before = _route_cost(route, cost_eval)
        delta, should_apply = op.evaluate(route[u], route[v], cost_eval)
        op.apply(route[u], route[v])
        route.update()
        actual = _route_cost(route, cost_eval) - before
        evaluations.append((delta, should_apply, actual))

    assert_equal(str(route), original)
    _assert_progress(evaluations)


@pytest.mark.parametrize("shipments", [False, True], ids=["clients", "pdptw"])
@pytest.mark.parametrize(
    ("epoch", "epsilon"),
    [(0, 2**-21), (100_000_000, 2**-15), (1_791_504_000, 2**-21)],
    ids=["absolute-tolerance", "relative-tolerance", "unix-timestamps"],
)
def test_time_window_relocation_cannot_cycle(epoch, epsilon, shipments):
    """
    Tolerant comparisons in DurationSegment::merge make regrouping the same
    visits change their time warp. Both C2 C1 C0 -> C2 C0 C1 and its reverse
    claim to save 1000 * epsilon, although the updated route costs are equal.
    The same happens when moving complete pickup/delivery pairs.

    Binary fractions make all the relevant arithmetic exactly representable:
    this failure comes from the comparison tolerance, not decimal rounding.
    """
    windows = [
        (2 + epsilon, 7 + 2 * epsilon),
        (2 + 3 * epsilon, 7 + 2 * epsilon),
        (3 + epsilon, 4),
    ]
    clients = []
    pairs = []
    for idx, (early, late) in enumerate(windows):
        if shipments:
            pairs.append(
                Shipment(
                    idx + 1,
                    idx + 1,
                    pickup_tw_early=epoch + early,
                    pickup_tw_late=epoch + late,
                )
            )
        else:
            clients.append(
                Client(idx + 1, tw_early=epoch + early, tw_late=epoch + late)
            )

    data = ProblemData(
        locations=[Location(0, 0) for _ in range(4)],
        clients=clients,
        shipments=pairs,
        depots=[Depot(0)],
        vehicle_types=[VehicleType(tw_early=epoch, tw_late=epoch + 100)],
        distance_matrices=[np.ones((4, 4)) - np.eye(4)],
        duration_matrices=[
            np.array([[0, 1, 3, 3], [0, 0, 3, 1], [2, 3, 0, 3], [0, 3, 3, 0]])
        ],
    )
    cost_eval = CostEvaluator([], 1000, 0)
    if shipments:
        # SearchSolution supplies linked pickup/delivery nodes.
        sol = SearchSolution(data)
        route = make_search_route(
            data, [*sol.shipments[2], *sol.shipments[1], *sol.shipments[0]]
        )
        _check_relocation_cycle(
            data, route, cost_eval, [(3, 6), (3, 6)], Relocate2
        )
    else:
        route = make_search_route(data, ["C2", "C1", "C0"])
        _check_relocation_cycle(data, route, cost_eval, [(2, 3), (2, 3)])


@pytest.mark.parametrize("variant", ["clients", "pdptw", "reload"])
@pytest.mark.parametrize("epoch", [0, 1_791_504_000], ids=["relative", "unix"])
@pytest.mark.parametrize("penalty", [1, 1_000, 100_000])
@pytest.mark.parametrize(
    ("service", "travel"),
    [(30, 60), (30.125, 60.25), (30.2, 60.2)],
    ids=["integer", "binary-fraction", "decimal-fraction"],
)
def test_unix_timestamp_neutral_relocation_cannot_cycle(
    variant, epoch, penalty, service, travel
):
    """
    Unix timestamps amplify rounding errors in DurationSegment::merge even
    with exact Measure comparisons. The timestamp is 2026-10-09 00:00:00 UTC;
    all client (or pickup) windows are five minutes wide, with approximately
    30 seconds of service and 60 seconds of travel between locations.

    Reordering identical clients, or complete pickup/delivery pairs, cannot
    change the route cost. With decimal durations and Unix timestamps, both
    directions nevertheless predict a time warp reduction of 2**-22 seconds.
    At penalties of 1000 and 100000, this exceeds TOL in objective units and
    both moves are accepted. The relative-time, exactly representable, and
    low-penalty cases are controls. Reloads do not prevent the cycle.
    """
    shipments = variant == "pdptw"
    reload = variant == "reload"
    num_locations = 6 if reload else 5
    clients = []
    pairs = []
    for idx in range(num_locations - 1):
        if shipments:
            pairs.append(
                Shipment(
                    idx + 1,
                    idx + 1,
                    pickup_tw_early=epoch,
                    pickup_tw_late=epoch + 300,
                    pickup_service_duration=service,
                    delivery_tw_early=epoch,
                    delivery_tw_late=epoch + 1000,
                )
            )
        else:
            clients.append(
                Client(
                    idx + 1,
                    tw_early=epoch,
                    tw_late=epoch + 300,
                    service_duration=service,
                )
            )

    durations = np.full((num_locations, num_locations), travel, dtype=float)
    np.fill_diagonal(durations, 0)
    data = ProblemData(
        locations=[Location(0, 0) for _ in range(num_locations)],
        clients=clients,
        shipments=pairs,
        depots=[Depot(0, tw_early=epoch, tw_late=epoch + 1000)],
        vehicle_types=[
            VehicleType(
                tw_early=epoch,
                tw_late=epoch + 1000,
                reload_depots=[0] if reload else [],
            )
        ],
        distance_matrices=[np.zeros_like(durations)],
        duration_matrices=[durations],
    )
    cost_eval = CostEvaluator([], penalty, 0)
    if shipments:
        sol = SearchSolution(data)
        route = make_search_route(
            data, [node for pair in sol.shipments for node in pair]
        )
        _check_relocation_cycle(
            data, route, cost_eval, [(1, 6), (5, 0)], Relocate2
        )
    else:
        visits = [f"C{idx}" for idx in range(4)]
        if reload:
            visits.extend(["D0", "C4"])
        route = make_search_route(data, visits)
        _check_relocation_cycle(data, route, cost_eval, [(1, 3), (3, 0)])


@pytest.mark.parametrize(
    "reload", [False, True], ids=["single-trip", "reload"]
)
@pytest.mark.parametrize("scale", [1, 1_000_000])
def test_mixed_pickup_delivery_relocation_cannot_cycle(scale, reload):
    """
    Tolerant max in LoadSegment::merge loses small increases differently for
    cached segments and the rebuilt route. Swapping the first two clients
    twice predicts -10 * epsilon both times, but actual deltas are
    +10 * epsilon and -10 * epsilon. Demands are around 2 and 3 (or millions),
    and are exact binary fractions. Reloading does not prevent the problem.
    """
    epsilon = 2**-20
    delivery = [2 * scale + 3 * epsilon, 3 * scale + epsilon, 3 * scale, scale]
    pickup = [
        2 * scale + 4 * epsilon,
        3 * scale + 2 * epsilon,
        3 * scale + 5 * epsilon,
        scale,
    ]
    num_clients = 4 if reload else 3
    data = ProblemData(
        locations=[Location(0, 0) for _ in range(num_clients + 1)],
        clients=[
            Client(idx + 1, delivery=[delivery[idx]], pickup=[pickup[idx]])
            for idx in range(num_clients)
        ],
        depots=[Depot(0)],
        vehicle_types=[
            VehicleType(
                capacity=[2 * scale], reload_depots=[0] if reload else []
            )
        ],
        distance_matrices=[np.zeros((num_clients + 1, num_clients + 1))],
        duration_matrices=[np.zeros((num_clients + 1, num_clients + 1))],
    )
    visits = ["C1", "C2", "C0"] + (["D0", "C3"] if reload else [])
    route = make_search_route(data, visits)
    _check_relocation_cycle(
        data, route, CostEvaluator([10], 0, 0), [(1, 2), (1, 2)]
    )


@pytest.mark.parametrize(
    "demand",
    [7.1, 57.1, 57.125],
    ids=["smaller-demand-control", "decimal", "binary-control"],
)
def test_capacity_only_neutral_relocation_cannot_cycle(demand):
    """
    Summing decimal demands in different groupings, then multiplying by a
    penalty, creates false improvements even without time windows or pickups.
    At the default maximum penalty of 1e5 and demand 57.1, the cost is about
    570.9 million, and both neutral moves report -1.07288e-6. Comparing each
    delta against zero loses the scale of its cost terms. Demand 7.1 is a
    smaller-scale control, and 57.125 is an exactly representable control.
    """
    num_clients = 100
    zeros = np.zeros((num_clients + 1, num_clients + 1))
    data = ProblemData(
        locations=[Location(0, 0) for _ in range(num_clients + 1)],
        clients=[
            Client(idx + 1, delivery=[demand]) for idx in range(num_clients)
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(capacity=[1])],
        distance_matrices=[zeros],
        duration_matrices=[zeros],
    )
    route = make_search_route(data, [f"C{idx}" for idx in range(num_clients)])
    _check_relocation_cycle(
        data, route, CostEvaluator([100_000], 0, 0), [(1, 26), (26, 0)]
    )


def _small_unit_cost_data(component, rate, optional=False):
    matrix = np.full((3, 3), 1_000_000.0)
    np.fill_diagonal(matrix, 0)
    zeros = np.zeros((3, 3))
    return ProblemData(
        locations=[Location(0, 0) for _ in range(3)],
        clients=[Client(1), Client(2, required=not optional)],
        depots=[Depot(0)],
        vehicle_types=[
            VehicleType(
                unit_distance_cost=rate if component == "distance" else 0,
                unit_duration_cost=rate if component == "duration" else 0,
            )
        ],
        distance_matrices=[matrix if component == "distance" else zeros],
        duration_matrices=[matrix if component == "duration" else zeros],
    )


@pytest.mark.parametrize("component", ["distance", "duration"])
@pytest.mark.parametrize("rate", [1e-7, 1e-5], ids=["small-rate", "control"])
def test_nonzero_unit_cost_neutral_relocation_cannot_cycle(component, rate):
    """
    hasDistanceCost/hasDurationCost compare the rate to zero using Measure's
    tolerance. At rate 1e-7 they skip the entire proposed cost, but the current
    route's cost is still subtracted. Both neutral swaps claim to save 0.3.
    """
    data = _small_unit_cost_data(component, rate)
    route = make_search_route(data, ["C0", "C1"])
    _check_relocation_cycle(
        data, route, CostEvaluator([], 0, 0), [(1, 2), (1, 2)]
    )


@pytest.mark.parametrize("component", ["distance", "duration"])
def test_optional_insertion_and_removal_cannot_both_improve(component):
    """
    Skipping a small nonzero unit cost also permits a cycle between different
    operators: insert a zero-prize optional client, then remove it. Insertion
    predicts -0.2 but costs +0.1; removal predicts -0.3 but saves only 0.1.
    """
    data = _small_unit_cost_data(component, 1e-7, optional=True)
    route = make_search_route(data, ["C0"])
    node = Node("C1")
    cost_eval = CostEvaluator([], 0, 0)
    evaluations = []
    before = _route_cost(route, cost_eval)

    insert = InsertOptionalClient(data)
    delta, should_apply = insert.evaluate(node, route[1], cost_eval)
    insert.apply(node, route[1])
    route.update()
    after = _route_cost(route, cost_eval)
    evaluations.append((delta, should_apply, after - before))

    remove = RemoveOptionalClient(data)
    delta, should_apply = remove.evaluate(node, cost_eval)
    remove.apply(node)
    route.update()
    evaluations.append(
        (delta, should_apply, _route_cost(route, cost_eval) - after)
    )

    assert_equal(str(route), "C0")
    assert_equal(_route_cost(route, cost_eval), before)
    _assert_progress(evaluations)


@pytest.mark.parametrize("num_load_dimensions", [0, 1])
@pytest.mark.parametrize("improvement", [0, TOL / 2, TOL, 2 * TOL])
def test_relocation_requires_improvement_beyond_tolerance(
    improvement, num_load_dimensions
):
    """
    Exact measure comparisons must not make sub-tolerance moves acceptable.
    With a load dimension, this also exercises CostEvaluator's early exit.
    """
    distances = np.zeros((3, 3))
    distances[0, 1] = improvement
    data = ProblemData(
        locations=[Location(0, 0) for _ in range(3)],
        clients=[
            Client(idx + 1, delivery=[0] * num_load_dimensions)
            for idx in range(2)
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(capacity=[1] * num_load_dimensions)],
        distance_matrices=[distances],
        duration_matrices=[np.zeros((3, 3))],
    )
    route = make_search_route(data, ["C0", "C1"])
    cost_eval = CostEvaluator([0] * num_load_dimensions, 0, 0)
    delta, should_apply = Relocate1(data).evaluate(
        route[1], route[2], cost_eval
    )

    # Reversing these clients removes the only positive-distance edge.
    assert_equal(delta, -improvement)
    assert_equal(should_apply, improvement > TOL)


@pytest.mark.parametrize("excess", [TOL / 2, TOL, 2 * TOL])
def test_feasibility_tolerance_preserves_computed_violations(excess):
    """
    Small load, distance, and duration violations must remain in the computed
    statistics, while feasibility decisions use an explicit tolerance.
    """
    matrix = np.array([[0, excess], [0, 0]])
    data = ProblemData(
        locations=[Location(0, 0), Location(0, 0)],
        clients=[Client(1, delivery=[excess])],
        depots=[Depot(0)],
        vehicle_types=[
            VehicleType(capacity=[0], max_distance=0, shift_duration=0)
        ],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
    )
    solution = Solution(data, [[0]])
    route = solution.routes()[0]
    search_route = make_search_route(data, ["C0"])
    for obj in (solution, route, search_route):
        assert_equal(obj.excess_load(), [excess])
        assert_equal(obj.excess_distance(), excess)
        assert_equal(obj.time_warp(), excess)
        assert_equal(obj.has_excess_load(), excess > TOL)
        assert_equal(obj.has_excess_distance(), excess > TOL)
        assert_equal(obj.has_time_warp(), excess > TOL)
        assert_equal(obj.is_feasible(), excess <= TOL)

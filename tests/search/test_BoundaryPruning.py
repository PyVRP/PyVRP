"""Correctness fixtures for conservative early boundary-cost rejection."""

import itertools

import numpy as np
import pytest
from numpy.testing import assert_allclose

from pyvrp import (
    Activity,
    ActivityType,
    Client,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    Route,
    Solution,
    VehicleType,
)
from pyvrp.search import _search as search
from tests.helpers import make_search_route


def test_early_bound_rejects_before_removal_evaluation(ok_small):
    data = ok_small.replace(vehicle_types=[VehicleType(2, capacity=[10])])
    op = search.Relocate1(data)
    first = make_search_route(data, ["C1", "C3", "C0", "C2"])
    second = make_search_route(data, [])
    delta, apply = op.evaluate(first[1], second[0], CostEvaluator([1], 1, 0))
    assert not apply
    assert delta == 0  # the earlier certificate, rather than partial 2346


def test_support_state_is_not_shared_between_problem_instances(ok_small):
    data = ok_small.replace(vehicle_types=[VehicleType(2, capacity=[10])])
    op = search.Relocate1(data)
    other = data.replace(
        vehicle_types=[VehicleType(2, capacity=[10], fixed_cost=1)]
    )
    other_op = search.Relocate1(other)
    assert other_op.name == "Relocate1"
    first = make_search_route(data, ["C1", "C3", "C0", "C2"])
    second = make_search_route(data, [])
    assert op.evaluate(first[1], second[0], CostEvaluator([1], 1, 0)) == (
        0,
        False,
    )


@pytest.mark.parametrize("scale", [0.1, 1.0, 1e6])
@pytest.mark.parametrize(
    "name,n,m",
    [
        ("Relocate1", 1, 0),
        ("Relocate2", 2, 0),
        ("Swap11", 1, 1),
        ("Swap21", 2, 1),
        ("Swap22", 2, 2),
    ],
)
def test_fractional_block_moves_against_full_routes(scale, name, n, m):
    matrix = np.array(
        [
            [
                0 if i == j else (1 + (i * 7 + j * 11 + i * j) % 17) * scale
                for j in range(5)
            ]
            for i in range(5)
        ]
    )
    data = ProblemData(
        [Location(i, 0) for i in range(5)],
        [
            Client(
                i + 1,
                delivery=[i + 1],
                service_duration=i + 1,
                tw_late=15 + 4 * i,
            )
            for i in range(4)
        ],
        [Depot(0)],
        [VehicleType(2, capacity=[5])],
        [matrix],
        [matrix],
    )

    def solution(sequences):
        return Solution(
            data,
            [
                Route(data, [Activity(ActivityType.CLIENT, i) for i in seq], 0)
                for seq in sequences
                if seq
            ],
        )

    for permutation in itertools.permutations(range(4)):
        for split in range(5):
            sequences = [list(permutation[:split]), list(permutation[split:])]
            nodes = [search.Node(ActivityType.CLIENT, i) for i in range(4)]
            routes = [search.Route(data, 0), search.Route(data, 0)]
            for route, seq in zip(routes, sequences):
                for idx in seq:
                    route.append(nodes[idx])
                route.update()
            for load, time in [(0, 0), (0.5, 0.75), (9.25, 11.75)]:
                evaluator = CostEvaluator([load], time, 0)
                op = getattr(search, name)(data)
                op.init(search.Solution(data))
                before = evaluator.penalised_cost(solution(sequences))
                for source in (0, 1):
                    target = 1 - source
                    for i in range(len(sequences[source]) - n + 1):
                        positions = (
                            range(len(sequences[target]) + 1)
                            if m == 0
                            else range(len(sequences[target]) - m + 1)
                        )
                        for j in positions:
                            candidate = [list(seq) for seq in sequences]
                            block = candidate[source][i : i + n]
                            if m == 0:
                                del candidate[source][i : i + n]
                                candidate[target][j:j] = block
                                v = routes[target][j]
                                skipped = False
                            else:
                                other = candidate[target][j : j + m]
                                candidate[source][i : i + n] = other
                                candidate[target][j : j + m] = block
                                v = nodes[sequences[target][j]]
                                skipped = (
                                    n == m
                                    and sequences[source][i]
                                    >= sequences[target][j]
                                )
                            delta, apply = op.evaluate(
                                nodes[sequences[source][i]], v, evaluator
                            )
                            exact = (
                                evaluator.penalised_cost(solution(candidate))
                                - before
                            )
                            # Away from tolerance, check both acceptance
                            # directions against full reconstruction.
                            tolerance = 1e-6 + 1e-12 * abs(before)
                            if abs(exact) > 10 * tolerance:
                                assert apply == (exact < 0 and not skipped)
                            if apply:
                                assert_allclose(
                                    delta, exact, rtol=1e-10, atol=tolerance
                                )


def test_emptying_route_with_fixed_cost_is_not_pruned():
    matrix = np.array([[0, 1, 1], [1, 0, 4], [1, 4, 0]], dtype=float)
    data = ProblemData(
        [Location(i, 0) for i in range(3)],
        [Client(1, delivery=[1]), Client(2, delivery=[1])],
        [Depot(0)],
        [VehicleType(2, capacity=[10], fixed_cost=10)],
        [matrix],
        [matrix],
    )
    nodes = [search.Node(ActivityType.CLIENT, i) for i in range(2)]
    routes = [search.Route(data, 0), search.Route(data, 0)]
    for route, node in zip(routes, nodes):
        route.append(node)
        route.update()
    op = search.Relocate1(data)
    op.init(search.Solution(data))
    delta, apply = op.evaluate(nodes[0], nodes[1], CostEvaluator([0], 0, 0))
    assert apply
    assert delta == -8  # +2 distance, but one less vehicle at fixed cost 10


@pytest.mark.parametrize("scale", [0.1, 1.0, 1e6])
def test_fractional_tail_moves_against_full_routes(scale):
    matrix = np.array(
        [
            [
                0 if i == j else (1 + (i * 7 + j * 11 + i * j) % 17) * scale
                for j in range(5)
            ]
            for i in range(5)
        ]
    )
    data = ProblemData(
        [Location(i, 0) for i in range(5)],
        [
            Client(
                i + 1,
                delivery=[i + 1],
                service_duration=i + 1,
                tw_late=15 + 4 * i,
            )
            for i in range(4)
        ],
        [Depot(0)],
        [VehicleType(2, capacity=[5])],
        [matrix],
        [matrix],
    )

    def cost(sequences, evaluator):
        routes = [
            Route(data, [Activity(ActivityType.CLIENT, i) for i in seq], 0)
            for seq in sequences
            if seq
        ]
        return evaluator.penalised_cost(Solution(data, routes))

    for permutation in itertools.permutations(range(4)):
        for split in (1, 2, 3):
            sequences = [list(permutation[:split]), list(permutation[split:])]
            nodes = [search.Node(ActivityType.CLIENT, i) for i in range(4)]
            routes = [search.Route(data, 0), search.Route(data, 0)]
            for route, seq in zip(routes, sequences):
                for idx in seq:
                    route.append(nodes[idx])
                route.update()
            for load, time in [(0, 0), (0.5, 0.75), (9.25, 11.75)]:
                evaluator = CostEvaluator([load], time, 0)
                before = cost(sequences, evaluator)
                op = search.SwapTails(data)
                for i in range(1, len(sequences[0]) + 1):
                    for j in range(1, len(sequences[1]) + 1):
                        candidate = [
                            sequences[0][:i] + sequences[1][j:],
                            sequences[1][:j] + sequences[0][i:],
                        ]
                        exact = cost(candidate, evaluator) - before
                        tolerance = 1e-6 + 1e-12 * abs(before)
                        results = [
                            op.evaluate(routes[0][i], routes[1][j], evaluator),
                            op.evaluate(routes[1][j], routes[0][i], evaluator),
                        ]
                        if abs(exact) > 10 * tolerance:
                            # Pointer ordering suppresses one orientation.
                            # Check both on the same native objects.
                            assert sum(apply for _, apply in results) == (
                                exact < 0
                            )
                        for delta, apply in results:
                            if apply:
                                assert_allclose(
                                    delta, exact, rtol=1e-10, atol=tolerance
                                )


@pytest.mark.parametrize("fixed_cost", [5e-7, 10])
def test_raw_model_guard_does_not_use_tolerant_zero(ok_small, fixed_cost):
    data = ok_small.replace(
        vehicle_types=[VehicleType(2, capacity=[10], fixed_cost=fixed_cost)]
    )
    first = make_search_route(data, ["C1", "C3", "C0", "C2"])
    second = make_search_route(data, [])
    delta, apply = search.Relocate1(data).evaluate(
        first[1], second[0], CostEvaluator([1], 1, 0)
    )
    assert not apply
    # Unsupported model: retain the original nonzero partial evaluation.
    assert delta > 0


def test_long_fractional_prefixes():
    count = 999
    grid = np.arange(count + 1, dtype=float)
    matrix = ((7 * grid[:, None] + 11 * grid[None, :]) % 17 + 1) * 1e8 + 0.1
    np.fill_diagonal(matrix, 0)
    data = ProblemData(
        [Location(i, 0) for i in range(count + 1)],
        [Client(i + 1, delivery=[1]) for i in range(count)],
        [Depot(0)],
        [VehicleType(2, capacity=[count])],
        [matrix],
        [np.zeros_like(matrix)],
    )
    nodes = [search.Node(ActivityType.CLIENT, i) for i in range(count)]
    routes = [search.Route(data, 0), search.Route(data, 0)]
    for i, node in enumerate(nodes):
        routes[int(i >= 500)].append(node)
    for route in routes:
        route.update()
    sequences = [list(range(500)), list(range(500, count))]
    evaluator = CostEvaluator([0], 0, 0)

    def cost(seqs):
        core = [
            Route(data, [Activity(ActivityType.CLIENT, i) for i in seq], 0)
            for seq in seqs
        ]
        return evaluator.penalised_cost(Solution(data, core))

    before = cost(sequences)
    for position in (0, 124, 498):
        candidate = [list(seq) for seq in sequences]
        moved = candidate[0].pop(position)
        candidate[1].insert(120, moved)
        exact = cost(candidate) - before
        op = search.Relocate1(data)
        op.init(search.Solution(data))
        delta, apply = op.evaluate(nodes[moved], routes[1][120], evaluator)
        if abs(exact) > 1:
            assert apply == (exact < 0)
        if apply:
            assert_allclose(delta, exact, rtol=1e-10, atol=0.01)

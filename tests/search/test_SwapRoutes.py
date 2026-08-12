import numpy as np
from numpy.testing import assert_, assert_equal

import pyvrp
from pyvrp import (
    Client,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    RandomNumberGenerator,
    VehicleType,
)
from pyvrp import Route as SolRoute
from pyvrp.search import LocalSearch, SwapRoutes
from pyvrp.search._search import Solution


def make_search_solution(data: ProblemData, routes: list[SolRoute]):
    sol = Solution(data)
    sol.load(pyvrp.Solution(data, routes))
    return sol


def test_swaps_complete_routes():
    """
    Tests swapping complete routes between two vehicle types.
    """
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
    solution = make_search_solution(data, routes)
    route1, route2 = solution.routes

    op = SwapRoutes(data)
    cost_evaluator = CostEvaluator([], 1, 0)
    result = op.evaluate(route1[1], route2[1], cost_evaluator)
    assert_equal(result, (-24, True))

    op.apply(route1[1], route2[1])
    route1.update()
    route2.update()

    assert_equal(str(route1), "C0")
    assert_equal(str(route2), "C1")
    assert_equal(route1.distance() + route2.distance(), 8)


def test_moves_complete_route_to_empty_vehicle():
    """
    Tests moving a complete route to an empty vehicle of another type.
    """
    data = ProblemData(
        locations=[Location(0, 0)],
        clients=[Client(0), Client(0)],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(1, fixed_cost=100), VehicleType(1)],
        distance_matrices=[np.zeros((1, 1), dtype=int)],
        duration_matrices=[np.zeros((1, 1), dtype=int)],
    )

    solution = make_search_solution(data, [SolRoute(data, [0, 1], 0)])
    route1, route2 = solution.routes

    op = SwapRoutes(data)
    cost_evaluator = CostEvaluator([], 0, 0)
    result = op.evaluate(route1[1], route2[0], cost_evaluator)
    assert_equal(result, (-100, True))

    op.apply(route1[1], route2[0])
    route1.update()
    route2.update()

    assert_equal(route1.num_clients(), 0)
    assert_equal(str(route2), "C0 C1")


def test_only_first_client_represents_nonempty_route(ok_small_two_profiles):
    """
    Tests that only the first client can represent a nonempty route.
    """
    data = ok_small_two_profiles
    solution = make_search_solution(
        data,
        [SolRoute(data, [0, 1], 0), SolRoute(data, [2, 3], 1)],
    )
    route1, route2 = solution.routes[0], solution.routes[3]

    op = SwapRoutes(data)
    cost_evaluator = CostEvaluator([1], 1, 1)
    assert_equal(op.evaluate(route1[2], route2[1], cost_evaluator), (0, False))


def test_local_search_swaps_routes():
    """
    Tests SwapRoutes through the local search.
    """
    data = ProblemData(
        locations=[
            Location(0, 0),
            Location(10, 0),
            Location(1, 0),
            Location(9, 0),
        ],
        clients=[Client(2), Client(3)],
        depots=[Depot(0), Depot(1)],
        vehicle_types=[
            VehicleType(start_depot=0, end_depot=0),
            VehicleType(start_depot=1, end_depot=1),
        ],
        distance_matrices=[
            [
                [0, 10, 1, 9],
                [10, 0, 9, 1],
                [1, 9, 0, 8],
                [9, 1, 8, 0],
            ]
        ],
        duration_matrices=[np.zeros((4, 4), dtype=int)],
    )

    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(data, rng, [[1], []])
    ls.add_operator(SwapRoutes(data))

    solution = pyvrp.Solution(
        data,
        [SolRoute(data, [1], 0), SolRoute(data, [0], 1)],
    )
    improved = ls(solution, CostEvaluator([], 1, 0), exhaustive=True)

    assert_equal(str(improved.routes()[0]), "C0")
    assert_equal(str(improved.routes()[1]), "C1")


def test_supports_at_least_two_vehicle_types(ok_small, ok_small_two_profiles):
    """
    Tests that SwapRoutes only supports heterogeneous fleets.
    """
    assert_(not SwapRoutes.supports(ok_small))
    assert_(SwapRoutes.supports(ok_small_two_profiles))

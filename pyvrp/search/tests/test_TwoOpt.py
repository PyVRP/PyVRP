from typing import List

import numpy as np
from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import (
    CostEvaluator,
    RandomNumberGenerator,
    Route,
    Solution,
    VehicleType,
)
from pyvrp.search import (
    LocalSearch,
    NeighbourhoodParams,
    TwoOpt,
    compute_neighbours,
)
from pyvrp.tests.helpers import make_heterogeneous, read


def test_OkSmall_instance():
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(TwoOpt(data))

    sol = Solution(data, [[1, 2, 3, 4]])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    assert_equal(improved_sol.num_routes(), 2)
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)

    # First improving (U, V) node pair is (1, 3), which results in the route
    # [1, 3, 2, 4]. The second improving node pair involves the depot of an
    # empty route: (1, 0). This results in routes [1] and [3, 2, 4].
    expected = Solution(data, [[1], [3, 2, 4]])
    assert_equal(improved_sol, expected)


@mark.parametrize(
    "vehicle_types",
    [
        [VehicleType(10, 1), VehicleType(10, 1)],
        [VehicleType(8, 1), VehicleType(10, 1)],
        [VehicleType(10, 1), VehicleType(8, 1)],
        [VehicleType(9, 1), VehicleType(9, 1)],
        [VehicleType(8, 1), VehicleType(8, 1)],
    ],
)
def test_OkSmall_heterogeneous_capacity(vehicle_types: List[VehicleType]):
    # This instance tests a two-opt move that is improving when disregarding
    # the vehicle capacities. Depending on the (heterogeneous) capacities,
    # the move may or may not be improving and should be applied or not
    # The starting solution has routes [1, 3] and [2, 4] with demands 8, 10
    # the 2-opt solution has routes [1, 4] and [2, 3] with demands 10, 8

    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(10000, 6)  # Large capacity penalty
    rng = RandomNumberGenerator(seed=42)

    # Now making it heterogenous, the same move should result in capacity
    # penalties and thus not be applied
    data = make_heterogeneous(data, vehicle_types)

    neighbours: List[List[int]] = [[], [2], [], [], []]  # only 1 -> 2
    ls = LocalSearch(data, rng, neighbours)
    ls.add_node_operator(TwoOpt(data))

    sol1 = Solution(data, [Route(data, [1, 3], 0), Route(data, [2, 4], 1)])
    sol2 = Solution(data, [Route(data, [1, 4], 0), Route(data, [2, 3], 1)])
    cost1 = cost_evaluator.penalised_cost(sol1)
    cost2 = cost_evaluator.penalised_cost(sol2)

    assert_(not np.allclose(cost1, cost2))

    # Using the local search, the result should not get worse
    improved_sol1 = ls.search(sol1, cost_evaluator)
    improved_sol2 = ls.search(sol2, cost_evaluator)

    expected_sol = sol1 if cost1 < cost2 else sol2
    assert_equal(improved_sol1, expected_sol)
    assert_equal(improved_sol2, expected_sol)


@mark.parametrize("seed", [2643, 2742, 2941, 3457, 4299, 4497, 6178, 6434])
def test_RC208_instance(seed: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=seed)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(TwoOpt(data))

    single_route = list(range(1, data.num_clients + 1))
    sol = Solution(data, [single_route])
    improved_sol = ls.search(sol, cost_evaluator)

    # The new solution should strictly improve on our original solution.
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)

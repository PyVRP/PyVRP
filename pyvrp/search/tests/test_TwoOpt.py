from typing import List

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
from pyvrp.tests.helpers import customise, read


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


def test_two_opt_heterogeneous_capacity():
    """
    Tests a two-opt move that is improving when considering homogeneous
    capacities, but is not improving when using heterogeneous capacities.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(10000, 0)  # penalise capacity, ignore tw
    rng = RandomNumberGenerator(seed=42)

    neighbours: List[List[int]] = [[], [2], [], [], []]  # only 1 -> 2
    ls = LocalSearch(data, rng, neighbours)
    ls.add_node_operator(TwoOpt(data))

    sol1 = Solution(data, [Route(data, [1, 3], 0), Route(data, [2, 4], 0)])
    sol2 = Solution(data, [Route(data, [1, 4], 0), Route(data, [2, 3], 0)])

    # Solution 1 is not locally optimal w.r.t. two-opt, but solution 2 is.
    assert_equal(ls.search(sol1, cost_evaluator), sol2)
    assert_equal(ls.search(sol2, cost_evaluator), sol2)

    # Now consider heterogeneous capacities 8 and 10.
    vehicle_types = [VehicleType(8, 1), VehicleType(10, 1)]
    data = customise(read("data/OkSmall.txt"), vehicle_types=vehicle_types)

    neighbours: List[List[int]] = [[], [2], [], [], []]
    ls = LocalSearch(data, rng, neighbours)
    ls.add_node_operator(TwoOpt(data))

    # The route demands of solution 1 are 8 and 10. The route demands of
    # solution 2 are 10 and 8, meaning that solution 2 is no longer feasible.
    sol1 = Solution(data, [Route(data, [1, 3], 0), Route(data, [2, 4], 1)])
    sol2 = Solution(data, [Route(data, [1, 4], 0), Route(data, [2, 3], 1)])

    # Now solution 1 is locally optimal w.r.t. two-opt, and solution 2 is not.
    assert_equal(ls.search(sol1, cost_evaluator), sol1)
    assert_equal(ls.search(sol2, cost_evaluator), sol1)


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

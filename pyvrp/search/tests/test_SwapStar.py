from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import CostEvaluator, Solution, XorShift128
from pyvrp.search import LocalSearch, SwapStar, compute_neighbours
from pyvrp.tests.helpers import read


@mark.parametrize("seed", [2643, 2742, 2941, 3457, 4299, 4497, 6178, 6434])
def test_swap_star_on_RC208_instance(seed: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=seed)

    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_route_operator(SwapStar(data))

    # Make an initial solution that consists of two routes, by randomly
    # splitting the single-route solution.
    route = list(range(1, data.num_clients + 1))
    split = rng.randint(data.num_clients)
    sol = Solution(data, [route[:split], route[split:]])
    improved_sol = ls.intensify(sol, cost_evaluator, overlap_tolerance=1)

    # The new solution should strictly improve on our original solution, but
    # cannot use more routes since SWAP* does not create routes.
    assert_equal(improved_sol.num_routes(), 2)
    current_cost = cost_evaluator.penalised_cost(sol)
    improved_cost = cost_evaluator.penalised_cost(improved_sol)
    assert_(improved_cost < current_cost)

from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import (
    Exchange11,
    LocalSearch,
    NeighbourhoodParams,
    SwapStar,
    compute_neighbours,
)
from pyvrp.tests.helpers import read


def test_swap_star_identifies_additional_moves_over_regular_swap():
    """
    SWAP* can move two clients to any position in the routes, whereas regular
    swap ((1, 1)-exchange) must reinsert each client in the other's position.
    Thus, SWAP* should still be able to identify additional improving moves
    after (1, 1)-exchange gets stuck.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    # For a fair comparison we should not hamper the node operator with
    # granularity restrictions.
    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, rng, compute_neighbours(data, nb_params))

    ls.add_node_operator(Exchange11(data))
    ls.add_route_operator(SwapStar(data))

    for _ in range(10):  # repeat a few times to really make sure
        sol = Solution.make_random(data, rng)

        swap_sol = ls.search(sol, cost_evaluator)
        swap_star_sol = ls.intensify(
            swap_sol, cost_evaluator, overlap_tolerance=1
        )

        # The regular swap operator should have been able to improve the random
        # solution. After swap gets stuck, SWAP* should still be able to
        # further improve the solution.
        current_cost = cost_evaluator.penalised_cost(sol)
        swap_cost = cost_evaluator.penalised_cost(swap_sol)
        swap_star_cost = cost_evaluator.penalised_cost(swap_star_sol)
        assert_(swap_cost < current_cost)
        assert_(swap_star_cost < swap_cost)


@mark.parametrize("seed", [2643, 2742, 2941, 3457, 4299, 4497, 6178, 6434])
def test_swap_star_on_RC208_instance(seed: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=seed)

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

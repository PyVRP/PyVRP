import numpy as np
from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import (
    Client,
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
    VehicleType,
)
from pyvrp.search import (
    Exchange11,
    LocalSearch,
    NeighbourhoodParams,
    SwapStar,
    compute_neighbours,
)
from pyvrp.search._search import Node, Route
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
    """
    Evaluate SWAP* on the RC208 instance, over a few seeds.
    """
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


def test_swap_star_can_swap_in_place():
    """
    It is not common that the best reinsert point of two nodes U and V from
    different routes is V for U and U for V, but SWAP* should be able to handle
    such a case. This is explicitly tested here because it is so rare.
    """
    data = ProblemData(
        clients=[
            Client(x=0, y=0),
            Client(x=1, y=1),
            Client(x=2, y=2),
            Client(x=3, y=3),
        ],
        vehicle_types=[VehicleType(capacity=0, num_available=2)],
        distance_matrix=np.asarray(
            [
                [0, 1, 10, 10],
                [1, 0, 10, 10],
                [10, 10, 0, 10],
                [10, 10, 1, 0],
            ]
        ),
        duration_matrix=np.zeros((4, 4), dtype=int),
    )

    nodes = [Node(loc=loc) for loc in range(data.num_clients + 1)]

    # First route is 0 -> 1 -> 2 -> 0.
    route1 = Route(data, idx=0, vehicle_type=0)
    route1.append(nodes[1])
    route1.append(nodes[2])
    route1.update()

    # Second route is 0 -> 3 -> 0.
    route2 = Route(data, idx=1, vehicle_type=0)
    route2.append(nodes[3])
    route2.update()

    cost_eval = CostEvaluator(1, 1)
    swap_star = SwapStar(data)

    # Best is to exchange clients 1 and 3. The cost delta is all distance: it
    # saves one expensive arc of cost 10, by replacing it with one of cost 1.
    assert_equal(swap_star.evaluate(route1, route2, cost_eval), -9)

    # Apply the move and test that it indeed swaps the nodes correctly.
    swap_star.apply(route1, route2)
    assert_(nodes[1].route is route2)
    assert_(nodes[3].route is route1)

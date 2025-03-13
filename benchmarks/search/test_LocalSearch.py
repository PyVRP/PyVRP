import pytest

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import (
    NODE_OPERATORS,
    ROUTE_OPERATORS,
    LocalSearch,
    compute_neighbours,
)


@pytest.mark.parametrize("instance", ["vrptw", "mdvrp", "vrpb"])
def test_all_operators(instance, benchmark, request):
    """
    Tests performance of the local search (with all default operators) on a few
    instances.
    """
    data = request.getfixturevalue(instance)
    rng = RandomNumberGenerator(seed=0)
    ls = LocalSearch(data, rng, compute_neighbours(data))

    for node_op in NODE_OPERATORS:
        ls.add_node_operator(node_op(data))

    for route_op in ROUTE_OPERATORS:
        ls.add_route_operator(route_op(data))

    sol = Solution.make_random(data, rng)
    cost_evaluator = CostEvaluator([20], 6, 6)
    benchmark(ls, sol, cost_evaluator)


def test_all_operators_on_vrptw_from_bks(benchmark, vrptw, vrptw_bks):
    """
    Tests all operators with the VRPTW instance, starting from the best-known
    solution, rather than a random one.
    """
    rng = RandomNumberGenerator(seed=0)
    ls = LocalSearch(vrptw, rng, compute_neighbours(vrptw))

    for node_op in NODE_OPERATORS:
        ls.add_node_operator(node_op(vrptw))

    for route_op in ROUTE_OPERATORS:
        ls.add_route_operator(route_op(vrptw))

    cost_evaluator = CostEvaluator([20], 6, 6)
    benchmark(ls, vrptw_bks, cost_evaluator)


@pytest.mark.parametrize("node_op", NODE_OPERATORS)
@pytest.mark.parametrize("instance", ["vrptw", "mdvrp", "vrpb"])
def test_each_node_operator(node_op, instance, benchmark, request):
    """
    Tests performance of each node operator on a few instances.
    """
    data = request.getfixturevalue(instance)
    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_node_operator(node_op(data))

    sol = Solution.make_random(data, rng)
    cost_evaluator = CostEvaluator([20], 6, 6)
    benchmark(ls, sol, cost_evaluator)


@pytest.mark.parametrize("route_op", ROUTE_OPERATORS)
@pytest.mark.parametrize("instance", ["vrptw", "mdvrp", "vrpb"])
def test_each_route_operator(route_op, instance, benchmark, request):
    """
    Tests performance of each route operator on a few instances.
    """
    data = request.getfixturevalue(instance)
    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_route_operator(route_op(data))

    sol = Solution.make_random(data, rng)
    cost_evaluator = CostEvaluator([20], 6, 6)
    benchmark(ls, sol, cost_evaluator)

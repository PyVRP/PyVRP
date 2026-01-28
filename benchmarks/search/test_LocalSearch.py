import pytest

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution
from pyvrp.search import OPERATORS, LocalSearch, compute_neighbours


@pytest.mark.parametrize("instance", ["vrptw", "mdvrp", "vrpb", "mtvrptwr"])
def test_all_operators(instance, benchmark, request):
    """
    Tests performance of the local search (with all default operators) on a few
    instances.
    """
    data = request.getfixturevalue(instance)
    rng = RandomNumberGenerator(seed=0)
    ls = LocalSearch(data, rng, compute_neighbours(data))

    for op in OPERATORS:
        if op.supports(data):
            ls.add_operator(op(data))

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

    for op in OPERATORS:
        if op.supports(vrptw):
            ls.add_operator(op(vrptw))

    cost_evaluator = CostEvaluator([20], 6, 6)
    benchmark(ls, vrptw_bks, cost_evaluator)


@pytest.mark.parametrize("op", OPERATORS)
@pytest.mark.parametrize("instance", ["vrptw", "mdvrp", "vrpb", "mtvrptwr"])
def test_each_operator(op, instance, benchmark, request):
    """
    Tests performance of each operator on a few instances.
    """
    data = request.getfixturevalue(instance)
    rng = RandomNumberGenerator(seed=42)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_operator(op(data))

    sol = Solution.make_random(data, rng)
    cost_evaluator = CostEvaluator([20], 6, 6)
    benchmark(ls, sol, cost_evaluator)

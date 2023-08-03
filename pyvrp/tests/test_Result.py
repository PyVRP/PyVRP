import math

from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises
from pytest import mark

from pyvrp import CostEvaluator, Population, RandomNumberGenerator, Solution
from pyvrp.Result import Result
from pyvrp.Statistics import Statistics
from pyvrp.diversity import broken_pairs_distance
from pyvrp.tests.helpers import read


@mark.parametrize(
    ("routes", "num_iterations", "runtime"),
    [([[1, 2], [3], [4]], 1, 1.5), ([[1, 2, 3, 4]], 100, 54.2)],
)
def test_fields_are_correctly_set(routes, num_iterations, runtime):
    data = read("data/OkSmall.txt")
    sol = Solution(data, routes)

    res = Result(sol, Statistics(), num_iterations, runtime)

    assert_equal(res.is_feasible(), sol.is_feasible())
    assert_equal(res.num_iterations, num_iterations)
    assert_allclose(res.runtime, runtime)

    if sol.is_feasible():
        assert_allclose(res.cost(), CostEvaluator().cost(sol))
    else:
        assert_equal(res.cost(), math.inf)


@mark.parametrize(
    ("num_iterations", "runtime"),
    [
        (-1, 0.0),  # num_iterations < 0
        (0, -1.0),  # runtime < 0
    ],
)
def test_init_raises_invalid_arguments(num_iterations, runtime):
    data = read("data/OkSmall.txt")
    sol = Solution(data, [[1, 2, 3, 4]])

    with assert_raises(ValueError):
        Result(sol, Statistics(), num_iterations, runtime)


@mark.parametrize("num_iterations", [0, 1, 10])
def test_num_iterations(num_iterations: int):
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)
    pop = Population(broken_pairs_distance)
    stats = Statistics()

    for _ in range(num_iterations):
        stats.collect_from(pop, cost_evaluator)

    best = Solution.make_random(data, rng)
    res = Result(best, stats, num_iterations, 0.0)
    assert_equal(res.num_iterations, num_iterations)


@mark.parametrize(
    "routes",
    [[[1, 2], [3], [4]], [[1, 2, 3, 4]]],
)
def test_str_contains_essential_information(routes):
    data = read("data/OkSmall.txt")

    sol = Solution(data, routes)
    res = Result(sol, Statistics(), 0, 0.0)
    str_representation = str(res)

    # Test that feasibility status and solution cost are presented.
    if sol.is_feasible():
        cost = CostEvaluator().cost(sol)
        assert_(str(cost) in str_representation)
    else:
        assert_("INFEASIBLE" in str_representation)

    # And make sure that all routes are printed as well.
    for route in sol.get_routes():
        assert_(str(route) in str_representation)

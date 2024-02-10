import math
import pickle

from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises
from pytest import mark

from pyvrp import CostEvaluator, Population, RandomNumberGenerator, Solution
from pyvrp.Result import Result
from pyvrp.Statistics import Statistics
from pyvrp.diversity import broken_pairs_distance


@mark.parametrize(
    ("routes", "num_iterations", "runtime"),
    [([[1, 2], [3], [4]], 1, 1.5), ([[1, 2, 3, 4]], 100, 54.2)],
)
def test_fields_are_correctly_set(ok_small, routes, num_iterations, runtime):
    """
    Tests that ``Result``'s data properties are correctly set after
    initialisation completes.
    """
    sol = Solution(ok_small, routes)
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
def test_init_raises_invalid_arguments(ok_small, num_iterations, runtime):
    """
    Tests that invalid arguments are rejected.
    """
    sol = Solution(ok_small, [[1, 2, 3, 4]])

    with assert_raises(ValueError):
        Result(sol, Statistics(), num_iterations, runtime)


@mark.parametrize("num_iterations", [0, 1, 10])
def test_num_iterations(ok_small, num_iterations: int):
    """
    Tests access to the ``num_iterations`` property.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)
    pop = Population(broken_pairs_distance)
    stats = Statistics()

    for _ in range(num_iterations):
        stats.collect_from(pop, cost_evaluator)

    best = Solution.make_random(ok_small, rng)
    res = Result(best, stats, num_iterations, 0.0)
    assert_equal(res.num_iterations, num_iterations)


@mark.parametrize(
    "routes",
    [[[1, 2], [3], [4]], [[1, 2, 3, 4]]],
)
def test_str_contains_essential_information(ok_small, routes):
    """
    Tests that printing (or, in general, calling ```str(result)``) returns a
    bunch of useful information about the underlying solution.
    """
    sol = Solution(ok_small, routes)
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


@mark.parametrize("num_iterations", [0, 1, 10])
def test_result_can_be_pickled(ok_small, num_iterations: int):
    """
    Tests that a ``Result`` object can be pickled: it can be serialised and
    unserialised. This is useful for e.g. storing results to disk.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)
    pop = Population(broken_pairs_distance)
    stats = Statistics()

    for _ in range(num_iterations):
        pop.add(Solution.make_random(ok_small, rng), cost_evaluator)
        stats.collect_from(pop, cost_evaluator)

    best = Solution(ok_small, [[1, 2], [3], [4]])

    before_pickle = Result(best, stats, num_iterations, 1.2)
    bytes = pickle.dumps(before_pickle)
    after_pickle = pickle.loads(bytes)

    assert_equal(after_pickle.best, before_pickle.best)
    assert_equal(after_pickle.stats, before_pickle.stats)
    assert_equal(after_pickle, before_pickle)

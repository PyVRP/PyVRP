from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises
from pytest import mark

from pyvrp import (
    GeneticAlgorithm,
    GeneticAlgorithmParams,
    PenaltyManager,
    Population,
    PopulationParams,
    Solution,
    XorShift128,
)
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.search import Exchange10, LocalSearch, compute_neighbours
from pyvrp.stop import MaxIterations
from pyvrp.tests.helpers import make_random_solutions, read, read_solution


@mark.parametrize(
    "repair_probability,"
    "collect_statistics,"
    "intensify_probability,"
    "intensify_on_best,"
    "nb_iter_no_improvement",
    [
        (-0.25, True, 0.5, True, 0),  # repair_probability < 0
        (1.25, True, 0.5, True, 0),  # repair_probability > 1
        (0.0, True, 0.5, True, -1),  # nb_iter_no_improvement < 0
    ],
)
def test_params_constructor_throws_when_arguments_invalid(
    repair_probability: float,
    collect_statistics: bool,
    intensify_probability: int,
    intensify_on_best: bool,
    nb_iter_no_improvement: int,
):
    """
    Tests that invalid configurations are not accepted.
    """
    with assert_raises(ValueError):
        GeneticAlgorithmParams(
            repair_probability,
            collect_statistics,
            intensify_probability,
            intensify_on_best,
            nb_iter_no_improvement,
        )


@mark.parametrize(
    "repair_probability,"
    "collect_statistics,"
    "intensify_probability,"
    "intensify_on_best,"
    "nb_iter_no_improvement",
    [
        (0.0, True, 0.5, True, 0),  # nb_iter_no_improvement == 0
        (0.0, True, 0.5, True, 1),  # repair_probability == 0
        (1.0, True, 0.5, True, 1),  # repair_probability == 1
        (0.5, False, 0.5, True, 1),  # collect_statistics is False
        (0.5, True, 0, True, 1),  # intensify_probability == 0
        (0.5, True, 1, True, 1),  # intensify_probability == 1
        (0.5, True, 0.5, False, 1),  # intensify_on_best is False
        (0.5, False, 0.5, False, 1),  # both False
    ],
)
def test_params_constructor_does_not_raise_when_arguments_valid(
    repair_probability: float,
    collect_statistics: bool,
    intensify_probability: float,
    intensify_on_best: bool,
    nb_iter_no_improvement: int,
):
    """
    Tests valid boundary cases.
    """
    params = GeneticAlgorithmParams(
        repair_probability,
        collect_statistics,
        intensify_probability,
        intensify_on_best,
        nb_iter_no_improvement,
    )

    assert_allclose(params.repair_probability, repair_probability)
    assert_equal(params.collect_statistics, collect_statistics)
    assert_equal(params.intensify_probability, intensify_probability)
    assert_equal(params.intensify_on_best, intensify_on_best)
    assert_equal(params.nb_iter_no_improvement, nb_iter_no_improvement)


def test_raises_when_no_initial_solutions():
    """
    Tests that GeneticAlgorithm raises when no initial solutions are provided,
    since that is insufficient to do crossover.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    pen_manager = PenaltyManager()
    rng = XorShift128(seed=42)
    ls = LocalSearch(data, rng, compute_neighbours(data))

    pop = Population(bpd)
    assert_equal(len(pop), 0)

    with assert_raises(ValueError):
        # No initial solutions should raise.
        GeneticAlgorithm(data, pen_manager, rng, pop, ls, srex, [])

    # One initial solution, so this should be OK.
    sol = Solution.make_random(data, rng)
    GeneticAlgorithm(data, pen_manager, rng, pop, ls, srex, [sol])


def test_initial_solutions_added_when_running():
    """
    Tests that GeneticAlgorithm adds initial solutions to the population
    when running the algorithm.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager()
    rng = XorShift128(seed=42)
    pop = Population(bpd)
    ls = LocalSearch(data, rng, compute_neighbours(data))
    init = set(make_random_solutions(25, data, rng))
    algo = GeneticAlgorithm(data, pm, rng, pop, ls, srex, init)

    algo.run(MaxIterations(0))

    # Since we ran the algorithm for zero iterations, the population should
    # contain only the initial solutions.
    current = {sol for sol in pop}
    assert_equal(len(current & init), 25)
    assert_equal(len(pop), 25)


def test_initial_solutions_added_when_restarting():
    """
    Tests that GeneticAlgorithm clears the population and adds the initial
    solutions when restarting.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager()
    rng = XorShift128(seed=42)
    pop = Population(bpd)

    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data))

    # We use the best known solution as one of the initial solutions so that
    # there are no improving iterations.
    init = {Solution(data, read_solution("data/RC208.sol"))}
    init.update(make_random_solutions(24, data, rng))

    params = GeneticAlgorithmParams(
        repair_probability=0,
        intensify_probability=0,
        intensify_on_best=False,
        nb_iter_no_improvement=100,
    )
    algo = GeneticAlgorithm(data, pm, rng, pop, ls, srex, init, params=params)

    algo.run(MaxIterations(100))

    # There are precisely enough non-improving iterations to trigger the
    # restarting mechanism. GA should have cleared and re-initialised the
    # population with the initial solutions.
    current = {sol for sol in pop}
    assert_equal(len(current & init), 25)

    # The population contains one more solution because of the search step.
    assert_equal(len(pop), 26)


def test_best_solution_improves_with_more_iterations():
    data = read("data/RC208.txt", "solomon", "dimacs")
    rng = XorShift128(seed=42)
    pm = PenaltyManager()
    pop_params = PopulationParams()
    pop = Population(bpd, params=pop_params)
    init = make_random_solutions(pop_params.min_pop_size, data, rng)

    ls = LocalSearch(data, rng, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data))

    ga_params = GeneticAlgorithmParams(
        intensify_probability=0, intensify_on_best=False
    )
    algo = GeneticAlgorithm(
        data, pm, rng, pop, ls, srex, init, params=ga_params
    )

    initial_best = algo.run(MaxIterations(0)).best
    new_best = algo.run(MaxIterations(25)).best

    cost_evaluator = pm.get_cost_evaluator()
    new_best_cost = cost_evaluator.penalised_cost(new_best)
    initial_best_cost = cost_evaluator.penalised_cost(initial_best)
    assert_(new_best_cost < initial_best_cost)
    assert_(new_best.is_feasible())  # best must be feasible


def test_best_initial_solution():
    """
    Tests that GeneticAlgorithm uses the best initial solution to initialise
    the best found solution.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    rng = XorShift128(seed=42)
    pm = PenaltyManager()
    pop = Population(bpd)

    bks = Solution(data, read_solution("data/RC208.sol"))
    init = [bks] + make_random_solutions(24, data, rng)

    ls = LocalSearch(data, rng, compute_neighbours(data))
    algo = GeneticAlgorithm(data, pm, rng, pop, ls, srex, init)

    result = algo.run(MaxIterations(0))

    # The best known solution is the best feasible initial solution. Since
    # we don't run any iterations, this should be returned as best solution.
    assert_equal(result.best, bks)


# TODO more functional tests

# TODO test statistics collection on Result.has_statistics

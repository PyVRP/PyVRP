from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises
from pytest import mark

from pyvrp import (
    GeneticAlgorithm,
    GeneticAlgorithmParams,
    Individual,
    PenaltyManager,
    Population,
    PopulationParams,
    XorShift128,
)
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.educate import Exchange10, LocalSearch, compute_neighbours
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
    pen_manager = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)
    ls = LocalSearch(data, pen_manager, rng, compute_neighbours(data))

    pop = Population(bpd)
    assert_equal(len(pop), 0)

    with assert_raises(ValueError):
        # No initial solutions should raise.
        GeneticAlgorithm(data, pen_manager, rng, pop, ls, srex, [])

    individual = Individual.make_random(data, pen_manager, rng)

    # One initial solution, so this should be OK.
    GeneticAlgorithm(data, pen_manager, rng, pop, ls, srex, [individual])


def test_initial_solutions_added_when_running():
    """
    Tests that GeneticAlgorithm adds initial solutions to the population
    when running the algorithm.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)
    pop = Population(bpd)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data))
    init = [Individual.make_random(data, pm, rng) for _ in range(25)]
    algo = GeneticAlgorithm(data, pm, rng, pop, ls, srex, init)

    algo.run(MaxIterations(0))

    # Check that the initial population individuals have the same routes as the
    # initial solutions.
    current = {individual for individual in pop}
    assert_equal(len(current & set(init)), 25)


def test_initial_solutions_added_when_restarting():
    """
    Tests that GeneticAlgorithm clears the population and adds the initial
    solutions when restarting.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)
    pop = Population(bpd)

    ls = LocalSearch(data, pm, rng, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data, pm))

    # We add the best known solution so that there are no improving iterations.
    best = Individual(data, pm, read_solution("data/RC208.sol"))
    init = [best] + [Individual.make_random(data, pm, rng) for _ in range(24)]

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
    current = {individual for individual in pop}
    assert_equal(len(current & set(init)), 25)

    # The population contains one more element because of the educate step.
    assert_equal(len(pop), 26)


def test_best_solution_improves_with_more_iterations():
    data = read("data/RC208.txt", "solomon", "dimacs")
    rng = XorShift128(seed=42)
    pm = PenaltyManager(data.vehicle_capacity)
    pop_params = PopulationParams()
    pop = Population(bpd, params=pop_params)
    init = make_random_solutions(pop_params.min_pop_size, data, pm, rng)

    ls = LocalSearch(data, pm, rng, compute_neighbours(data))
    ls.add_node_operator(Exchange10(data, pm))

    ga_params = GeneticAlgorithmParams(
        intensify_probability=0, intensify_on_best=False
    )
    algo = GeneticAlgorithm(
        data, pm, rng, pop, ls, srex, init, params=ga_params
    )

    initial_best = algo.run(MaxIterations(0)).best
    new_best = algo.run(MaxIterations(25)).best

    assert_(new_best.cost() < initial_best.cost())
    assert_(new_best.is_feasible())  # best must be feasible


def test_best_initial_solution():
    """
    Tests that GeneticAlgorithm uses the best initial solution to initialise
    the best found solution.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    rng = XorShift128(seed=42)
    pm = PenaltyManager(data.vehicle_capacity)
    pop_params = PopulationParams()
    pop = Population(bpd, params=pop_params)
    init = make_random_solutions(pop_params.min_pop_size, data, pm, rng)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data))
    algo = GeneticAlgorithm(data, pm, rng, pop, ls, srex, init)

    result = algo.run(MaxIterations(0))
    best_init = min(
        init, key=lambda indiv: (not indiv.is_feasible(), indiv.cost())
    )

    assert_equal(result.best, best_init)


# TODO more functional tests

# TODO test statistics collection on Result.has_statistics

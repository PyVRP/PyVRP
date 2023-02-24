from numpy.testing import (
    assert_,
    assert_almost_equal,
    assert_equal,
    assert_raises,
)
from pytest import mark

from pyvrp import (
    GeneticAlgorithm,
    GeneticAlgorithmParams,
    PenaltyManager,
    Population,
    XorShift128,
)
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.educate import Exchange10, LocalSearch, compute_neighbours
from pyvrp.stop import MaxIterations
from pyvrp.tests.helpers import read


@mark.parametrize(
    "repair_probability,"
    "collect_statistics,"
    "intensification_probability,"
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
    intensification_probability: int,
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
            intensification_probability,
            intensify_on_best,
            nb_iter_no_improvement,
        )


@mark.parametrize(
    "repair_probability,"
    "collect_statistics,"
    "intensification_probability,"
    "intensify_on_best,"
    "nb_iter_no_improvement",
    [
        (0.0, True, 0.5, True, 0),  # nb_iter_no_improvement == 0
        (0.0, True, 0.5, True, 1),  # repair_probability == 0
        (1.0, True, 0.5, True, 1),  # repair_probability == 1
        (0.5, False, 0.5, True, 1),  # collect_statistics is False
        (0.5, True, 0, True, 1),  # intensification_probability == 0
        (0.5, True, 1, True, 1),  # intensification_probability == 1
        (0.5, True, 0.5, False, 1),  # intensify_on_best is False
        (0.5, False, 0.5, False, 1),  # both False
    ],
)
def test_params_constructor_does_not_raise_when_arguments_valid(
    repair_probability: float,
    collect_statistics: bool,
    intensification_probability: int,
    intensify_on_best: bool,
    nb_iter_no_improvement: int,
):
    """
    Tests valid boundary cases.
    """
    params = GeneticAlgorithmParams(
        repair_probability,
        collect_statistics,
        intensification_probability,
        intensify_on_best,
        nb_iter_no_improvement,
    )

    assert_almost_equal(params.repair_probability, repair_probability)
    assert_equal(params.collect_statistics, collect_statistics)
    assert_equal(params.intensification_probability, intensify_on_best)
    assert_equal(params.intensify_on_best, intensify_on_best)
    assert_equal(params.nb_iter_no_improvement, nb_iter_no_improvement)


def test_best_solution_improves_with_more_iterations():
    data = read("data/RC208.txt", "solomon", "dimacs")
    rng = XorShift128(seed=42)
    pen_manager = PenaltyManager(data.vehicle_capacity)
    pop = Population(data, pen_manager, rng, bpd)
    ls = LocalSearch(data, pen_manager, rng, compute_neighbours(data))

    node_op = Exchange10(data, pen_manager)
    ls.add_node_operator(node_op)

    params = GeneticAlgorithmParams(should_intensify=False)
    algo = GeneticAlgorithm(data, pen_manager, rng, pop, ls, srex, params)

    initial_best = algo.run(MaxIterations(0)).best
    new_best = algo.run(MaxIterations(25)).best

    assert_(new_best.cost() < initial_best.cost())
    assert_(new_best.is_feasible())  # best must be feasible


# TODO more functional tests

# TODO test statistics collection on Result.has_statistics

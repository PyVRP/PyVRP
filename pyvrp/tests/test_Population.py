from numpy.testing import (
    assert_,
    assert_almost_equal,
    assert_equal,
    assert_raises,
)
from pytest import mark

from pyvrp import Population, PopulationParams
from pyvrp._lib.hgspy import Individual, PenaltyManager, XorShift128
from pyvrp.diversity import broken_pairs_distance
from pyvrp.tests.helpers import read


@mark.parametrize(
    "min_pop_size,"
    "generation_size,"
    "nb_elite,"
    "nb_close,"
    "lb_diversity,"
    "ub_diversity",
    [
        (-1, 1, 1, 1, 0.0, 1.0),  # -1 min_pop_size
        (1, -1, 1, 1, 0.0, 1.0),  # -1 generation_size
        (1, 1, -1, 1, 0.0, 1.0),  # -1 nb_elite
        (1, 1, 1, -1, 0.0, 1.0),  # -1 nb_close
        (1, 1, 1, -1, -1, 1.0),  # -1 lb_diversity
        (1, 1, 1, -1, 2, 1.0),  # 2 lb_diversity
        (1, 1, 1, -1, 0, -1.0),  # -1 ub_diversity
        (1, 1, 1, -1, 0, 2.0),  # 2 ub_diversity
        (1, 1, 1, -1, 1, 0.5),  # ub_diversity < lb_diversity
        (1, 1, 1, -1, 0.5, 0.5),  # ub_diversity == lb_diversity
    ],
)
def test_params_constructor_throws_when_arguments_invalid(
    min_pop_size: int,
    generation_size: int,
    nb_elite: int,
    nb_close: int,
    lb_diversity: float,
    ub_diversity: float,
):
    """
    Tests that invalid configurations are not accepted.
    """
    with assert_raises(ValueError):
        PopulationParams(
            min_pop_size,
            generation_size,
            nb_elite,
            nb_close,
            lb_diversity,
            ub_diversity,
        )


@mark.parametrize(
    "min_pop_size,"
    "generation_size,"
    "nb_elite,"
    "nb_close,"
    "lb_diversity,"
    "ub_diversity",
    [
        (1, 1, 1, 1, 0.0, 0.5),  # >0 min_pop_size
        (1, 0, 1, 1, 0.0, 0.5),  # 0 generation_size
        (1, 1, 0, 1, 0.0, 0.5),  # 0 nb_elite
        (1, 1, 1, 0, 0.0, 0.5),  # 0 nb_close
        (1, 1, 1, 1, 0.0, 0.5),  # 0 lb_diversity
        (1, 1, 1, 1, 0.0, 1.0),  # 1 ub_diversity
    ],
)
def test_params_constructor_does_not_raise_when_arguments_valid(
    min_pop_size: int,
    generation_size: int,
    nb_elite: int,
    nb_close: int,
    lb_diversity: float,
    ub_diversity: float,
):
    """
    Tests valid boundary cases.
    """
    params = PopulationParams(
        min_pop_size,
        generation_size,
        nb_elite,
        nb_close,
        lb_diversity,
        ub_diversity,
    )

    assert_equal(params.min_pop_size, min_pop_size)
    assert_equal(params.generation_size, generation_size)
    assert_equal(params.nb_elite, nb_elite)
    assert_equal(params.nb_close, nb_close)
    assert_almost_equal(params.lb_diversity, lb_diversity)
    assert_almost_equal(params.ub_diversity, ub_diversity)
    assert_equal(params.max_pop_size, min_pop_size + generation_size)


# TODO functional tests


def test_initialises_at_least_min_pop_size_individuals():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    params = PopulationParams()
    pop = Population(data, pm, rng, broken_pairs_distance)

    assert_(len(pop) >= params.min_pop_size)


def test_add_triggers_purge():
    pass  # TODO


# TEST(PopulationTest, addTriggersPurge)
# {
#     auto const data = ProblemData::fromFile("data/OkSmall.txt");
#     PenaltyManager pMngr(data.vehicleCapacity());
#     XorShift128 rng;

#     PopulationParams params;
#     Population pop(data, pMngr, rng, brokenPairsDistance, params);

#     // After construction, we should have minPopSize individuals.
#     EXPECT_EQ(pop.size(), params.minPopSize);

#     size_t infeasPops = pop.numInfeasible();
#     size_t feasPops = pop.numFeasible();

#     EXPECT_EQ(pop.size(), infeasPops + feasPops);

#     while (true)  // keep adding feasible individuals until we are about to
#  do
#     {             // survivor selection.
#         Individual indiv = {data, pMngr, rng};

#         if (indiv.isFeasible())
#         {
#             pop.add(indiv);
#             feasPops++;

#             EXPECT_EQ(pop.size(), infeasPops + feasPops);
#             EXPECT_EQ(pop.numFeasible(), feasPops);
#         }

#         if (feasPops == params.minPopSize + params.generationSize)
#             break;
#     }

#     // RNG is fixed, and this next individual is feasible. Since we now have
#  a
#     // feasible population size of minPopSize + generationSize, adding this
#  new
#     // individual should trigger survivor selection. Survivor selection
#  reduces
#     // the feasible sub-population to minPopSize, so the overall population
# is
#     // just infeasPops + minPopSize.
#     Individual indiv = {data, pMngr, rng};
#     pop.add(indiv);

#     ASSERT_TRUE(indiv.isFeasible());
#     EXPECT_EQ(pop.numFeasible(), params.minPopSize);
#     EXPECT_EQ(pop.size(), params.minPopSize + infeasPops);
# }


def test_add_updates_best_found_solution():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=2_147_483_647)

    params = PopulationParams(0, 40, 4, 5, 0.1, 0.5)
    pop = Population(data, pm, rng, broken_pairs_distance, params)

    # Should not have added any individuals to the population pool. The 'best'
    # individual, however, has already been initialised, with a random
    # individual.
    assert_equal(len(pop), 0)

    # This random individual is feasible and has cost 9'339.
    best = pop.get_best_found()
    assert_almost_equal(best.cost(), 9_339)
    assert_(best.is_feasible())

    # We now add a better solution to the population.
    pop.add(Individual(data, pm, [[3, 2], [1, 4], []]))
    assert_equal(len(pop), 1)

    best = pop.get_best_found()
    assert_almost_equal(best.cost(), 9_155)
    assert_(best.is_feasible())


# TODO test more add() - fitness, duplicate, purge


def test_select_returns_same_parents_if_no_other_option():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=2_147_483_647)

    params = PopulationParams(0, 40, 4, 5, 0.1, 0.5)
    pop = Population(data, pm, rng, broken_pairs_distance, params)

    assert_equal(len(pop), 0)

    pop.add(Individual(data, pm, [[3, 2], [1, 4], []]))
    assert_equal(len(pop), 1)

    # We added a single individual, so we should now get the same parent twice.
    parents = pop.select()
    assert_(parents[0] == parents[1])

    # Now we add another, different individual.
    pop.add(Individual(data, pm, [[3, 2], [1], [4]]))
    assert_equal(len(pop), 2)

    # We should now get two different individuals as parents, at least most of
    # the time. The actual probability of getting the same parents is very
    # small, but not zero. So let's do an experiment where we do 1000 selects,
    # and collect the number of times the parents are different.
    different_parents = 0
    for _ in range(1_000):
        parents = pop.select()
        different_parents += parents[0] != parents[1]

    # The probability of selecting different parents is very close to 100%, so
    # we would expect to observe different parents much more than 90% of the
    # time. At the same time, it is very unlikely each one of the 1000 selects
    # returns a different parent pair.
    assert_(900 < different_parents < 1_000)


# // TODO test more select() - diversity, feas/infeas pairs


@mark.parametrize("min_pop_size", [0, 2, 5, 10])
def test_pop_and_proximity_are_in_sync(min_pop_size: int):
    """
    This test checks that the population's feasible and infeasible individuals
    are in sync with the population's proximity structure.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    params = PopulationParams(min_pop_size=min_pop_size)
    pop = Population(data, pm, rng, broken_pairs_distance, params)

    feas = pop.feasible_subpopulation
    infeas = pop.infeasible_subpopulation
    prox = pop.proximity_structure

    # We run a few times the maximum pop size, to make sure that we get one or
    # more purge cycles in.
    for _ in range(5 * params.max_pop_size):
        indiv = Individual(data, pm, rng)
        pop.add(indiv)

        # The proximity structure should have the same size as the current
        # total population.
        assert_equal(len(pop), len(prox))

        for indiv in feas:
            # Each individual should be in the proximity structure, and have a
            # proximity value for every other individual in the same
            # subpopulation (so there should be n - 1 such values).
            assert_(indiv in prox)
            assert_equal(len(prox[indiv]), len(feas) - 1)

        for indiv in infeas:
            # The same must hold for the infeasible subpopulation, of course!
            assert_(indiv in prox)
            assert_equal(len(prox[indiv]), len(infeas) - 1)

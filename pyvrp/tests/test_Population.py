import numpy as np
from numpy.testing import (
    assert_,
    assert_almost_equal,
    assert_equal,
    assert_raises,
)
from pytest import mark

from pyvrp import (
    Individual,
    PenaltyManager,
    Population,
    PopulationParams,
    XorShift128,
)
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
        (1, 1, 1, 1, -1, 1.0),  # -1 lb_diversity
        (1, 1, 1, 1, 2, 1.0),  # 2 lb_diversity
        (1, 1, 1, 1, 0, -1.0),  # -1 ub_diversity
        (1, 1, 1, 1, 0, 2.0),  # 2 ub_diversity
        (1, 1, 1, 1, 1, 0.5),  # ub_diversity < lb_diversity
        (1, 1, 1, 1, 0.5, 0.5),  # ub_diversity == lb_diversity
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


def test_add_triggers_purge():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    params = PopulationParams()
    pop = Population(data, pm, rng, broken_pairs_distance, params)

    # Population should initialise at least min_pop_size individuals
    assert_(len(pop) >= params.min_pop_size)
    assert_equal(len(pop), pop.num_feasible() + pop.num_infeasible())

    num_feas = pop.num_feasible()
    num_infeas = pop.num_infeasible()

    while True:  # keep adding feasible individuals until we are about to purge
        individual = Individual(data, pm, rng)

        if individual.is_feasible():
            pop.add(individual)
            num_feas += 1

            assert_equal(len(pop), num_feas + num_infeas)
            assert_equal(pop.num_feasible(), num_feas)

        if num_feas == params.max_pop_size:  # next add() triggers purge
            break

    # RNG is fixed, and this next individual is feasible. Since we now have a
    # feasible population that is of maximal size, adding this individual
    # should trigger survivor selection (purge). Survivor selection reduces the
    # feasible subpopulation to min_pop_size, so the overal population is then
    # just num_infeas + min_pop_size.
    individual = Individual(data, pm, rng)
    assert_(individual.is_feasible())

    pop.add(individual)
    assert_equal(pop.num_feasible(), params.min_pop_size)
    assert_equal(len(pop), num_infeas + params.min_pop_size)


# TODO test more add() - fitness, duplicate, purge


def test_select_returns_same_parents_if_no_other_option():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=2_147_483_647)

    params = PopulationParams(min_pop_size=0)
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


def test_restart_generates_min_pop_size_new_individuals():
    """
    Tests if restarting the population will generate ``min_pop_size`` new
    individuals.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=12)

    params = PopulationParams()
    pop = Population(data, pm, rng, broken_pairs_distance, params)

    old = {individual for individual in pop}
    pop.restart()
    new = {individual for individual in pop}

    assert_equal(len(pop), params.min_pop_size)
    print(old, new)
    assert_equal(len(old & new), 0)  # no old pops survived the restart


def test_population_is_empty_with_zero_min_pop_size_and_generation_size():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=12)

    params = PopulationParams(min_pop_size=0, generation_size=0)
    pop = Population(data, pm, rng, broken_pairs_distance, params)

    assert_equal(len(pop), 0)

    for _ in range(10):
        # With zero min_pop_size and zero generation_size, every additional
        # individual triggers a purge. So the population size can never grow
        # beyond zero.
        pop.add(Individual(data, pm, rng))
        assert_equal(len(pop), 0)


@mark.parametrize("nb_elite", [5, 25])
def test_elite_individuals_are_not_purged(nb_elite: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.num_vehicles)
    params = PopulationParams(nb_elite=nb_elite)
    rng = XorShift128(seed=42)

    pop = Population(data, pm, rng, broken_pairs_distance, params)

    # Keep adding individuals until the infeasible subpopulation is of maximum
    # size.
    while pop.num_infeasible() != params.max_pop_size:
        pop.add(Individual(data, pm, rng))

    assert_equal(pop.num_infeasible(), params.max_pop_size)

    # These are the nb_elite best solutions in the current solution pool. These
    # should never be purged.
    curr_individuals = [
        individual for individual in pop if not individual.is_feasible()
    ]

    best_individuals = sorted(curr_individuals, key=lambda indiv: indiv.cost())
    elite_individuals = best_individuals[:nb_elite]

    # Add a solution that is certainly not feasible, thus causing a purge.
    single_route = [client for client in range(1, data.num_clients + 1)]
    pop.add(Individual(data, pm, [single_route]))

    # After the purge, there should remain min_pop_size infeasible solutions.
    assert_equal(pop.num_infeasible(), params.min_pop_size)

    # In the infeasible subpopulation, nb_elite solutions from before the purge
    # should also still be present. We test that by selecting the nb_elite best
    # individuals from before the purge. We test by id/memory location because
    # these individuals should still be present, unmodified.
    new_individuals = [id(individual) for individual in pop]
    for elite_individual in elite_individuals:
        assert_(id(elite_individual) in new_individuals)


def test_binary_tournament_ranks_by_fitness():
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.num_vehicles)
    params = PopulationParams()
    rng = XorShift128(seed=42)

    pop = Population(data, pm, rng, broken_pairs_distance, params)

    for _ in range(50):
        pop.add(Individual(data, pm, rng))

    assert_equal(pop.num_feasible(), 0)

    # Since this test requires the fitness values of the individuals, we have
    # to access the underlying infeasible subpopulation directly.
    infeas = [item for item in pop._infeas]
    infeas = sorted(infeas, key=lambda item: item.fitness)
    infeas = {item.individual: idx for idx, item in enumerate(infeas)}
    infeas_count = np.zeros(len(infeas))

    for _ in range(10_000):
        indiv = pop.get_binary_tournament()
        infeas_count[infeas[indiv]] += 1

    # Now we compare the observed ranking from the binary tournament selection
    # against what we would expect from the actual fitness ranking. We compute
    # the percentage of times we're incorrect, and test that that number is not
    # too high.
    actual_rank = np.argsort(-infeas_count)  # higher is better
    expected_rank = np.arange(len(infeas))
    pct_off = np.abs((actual_rank - expected_rank) / len(infeas)).mean()

    assert_(pct_off < 0.05)

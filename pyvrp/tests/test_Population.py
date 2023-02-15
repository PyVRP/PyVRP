from typing import NamedTuple

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
from pyvrp.Population import GenericPopulation
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


@mark.parametrize("use_numpy", [False, True])
def test_add_triggers_purge(use_numpy: bool):
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    params = PopulationParams(use_numpy=use_numpy)
    pop = Population(data, pm, rng, broken_pairs_distance, params=params)

    # Population should initialise at least min_pop_size individuals
    assert_(len(pop) >= params.min_pop_size)

    num_feas = len(pop.feasible_subpopulation)
    num_infeas = len(pop.infeasible_subpopulation)

    assert_equal(len(pop), num_feas + num_infeas)

    for _ in range(3):
        while (
            True
        ):  # keep adding feasible individuals until we are about to purge
            individual = Individual(data, pm, rng)

            if individual.is_feasible():
                pop.add(individual)
                num_feas += 1

                assert_equal(len(pop), num_feas + num_infeas)
                assert_equal(len(pop.feasible_subpopulation), num_feas)

            if (
                num_feas == params.max_pop_size - 1
            ):  # next add() triggers purge
                break

        # Since we now have a feasible population that is of maximal size,
        # adding an individual should trigger survivor selection (purge).
        # Survivor selection reduces the feasible subpopulation to
        # min_pop_size, so the overal population is then just num_infeas +
        # min_pop_size.

        # Find a feasible individual (should not take forever)
        individual = Individual(data, pm, rng)
        while not individual.is_feasible():
            individual = Individual(data, pm, rng)

        assert_(individual.is_feasible())

        pop.add(individual)
        assert_equal(len(pop.feasible_subpopulation), params.min_pop_size)
        assert_equal(len(pop), num_infeas + params.min_pop_size)

        num_feas = params.min_pop_size


@mark.parametrize("use_numpy", [False, True])
def test_add_updates_best_found_solution(use_numpy: bool):
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=2_133_234_000)

    params = PopulationParams(min_pop_size=0, use_numpy=use_numpy)
    pop = Population(data, pm, rng, broken_pairs_distance, params=params)

    # Should not have added any individuals to the population pool. The 'best'
    # individual, however, has already been initialised, with a random
    # individual.
    assert_equal(len(pop), 0)

    # This random individual is feasible and has cost > 9_155 (see below).
    best = pop.get_best_found()
    assert_(best.cost() > 9_155)
    assert_(best.is_feasible())

    # We now add a better solution to the population.
    pop.add(Individual(data, pm, [[3, 2], [1, 4], []]))
    assert_equal(len(pop), 1)

    new_best = pop.get_best_found()
    assert_almost_equal(new_best.cost(), 9_155)
    assert_(new_best.cost() < best.cost())
    assert_(best.is_feasible())


# TODO test more add() - fitness, duplicate, purge


@mark.parametrize("use_numpy", [False, True])
def test_select_returns_same_parents_if_no_other_option(use_numpy: bool):
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=2_147_483_647)

    params = PopulationParams(min_pop_size=0, use_numpy=use_numpy)
    pop = Population(data, pm, rng, broken_pairs_distance, params=params)

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
@mark.parametrize(
    "use_numpy",
    [False, True],
)
def test_custom_initial_solutions(use_numpy: bool):
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=2_147_483_647)

    params = PopulationParams(min_pop_size=0, use_numpy=use_numpy)

    initial_solutions = [
        Individual(data, pm, [[3, 2], [1, 4], []]),
        Individual(data, pm, [[3, 2], [1], [4]]),
    ]
    pop = Population(
        data,
        pm,
        rng,
        broken_pairs_distance,
        initial_solutions=initial_solutions,
        params=params,
    )

    assert_equal(len(pop), 2)

    pop.add(Individual(data, pm, [[3, 2], [1, 4], []]))
    assert_equal(len(pop), 3)


@mark.parametrize("use_numpy", [False, True])
@mark.parametrize("nb_elite", [5, 25])
def test_elite_not_purged_computation(nb_elite: int, use_numpy: bool):

    # Helper functions
    def has_duplicates(vals: np.ndarray):
        sorted_vals = np.sort(vals)
        return np.isclose(sorted_vals[:-1], sorted_vals[1:]).any()

    def get_rank(vals: np.ndarray[float]):
        rank = np.zeros(len(vals), dtype=int)
        rank[np.argsort(vals)] = np.arange(len(vals))
        return rank

    # TODO setup as fixture
    class TestIndividual(NamedTuple):
        idx: int
        costval: float

        def cost(self) -> float:
            return self.costval

        def is_feasible(self) -> bool:
            return True

    rng = np.random.default_rng(seed=2_147_483_647)

    n = 100
    costs = rng.random(n)
    dists = rng.random((n, n))
    dists = np.maximum(dists, dists.T)  # Symmetrize

    indivs = [TestIndividual(i, c) for i, c in enumerate(costs)]

    def get_diversity(indiv1: TestIndividual, indiv2: TestIndividual):
        return dists[indiv1.idx, indiv2.idx]

    params = PopulationParams(
        use_numpy=use_numpy,
        min_pop_size=25,
        generation_size=n - 25,
        nb_elite=nb_elite,
    )
    pop = GenericPopulation[TestIndividual](
        indivs[0], get_diversity, rng.integers, params=params
    )

    for indiv in indivs:
        pop.add(indiv)

    # Test that we have purged indeed
    assert_equal(params.min_pop_size, len(pop))

    # Make sure elite individuals are among these
    for elite_idx in np.argsort(costs)[: params.nb_elite]:
        # Note: O(n) not most efficient way to check but will do for testing
        assert_(indivs[elite_idx] in pop.feasible_subpopulation)


@mark.parametrize("use_numpy", [False, True])
def test_biased_fitness_computation(use_numpy: bool):

    # Helper functions
    def has_duplicates(vals: np.ndarray):
        sorted_vals = np.sort(vals)
        return np.isclose(sorted_vals[:-1], sorted_vals[1:]).any()

    def get_rank(vals: np.ndarray[float]):
        rank = np.zeros(len(vals), dtype=int)
        rank[np.argsort(vals)] = np.arange(len(vals))
        return rank

    # TODO setup as fixture
    class TestIndividual(NamedTuple):
        idx: int
        costval: float

        def cost(self) -> float:
            return self.costval

        def is_feasible(self) -> bool:
            return True

    rng = np.random.default_rng(seed=2_147_483_647)

    n = 100
    costs = rng.random(n)
    dists = rng.random((n, n))
    dists = np.maximum(dists, dists.T)  # Symmetrize

    indivs = [TestIndividual(i, c) for i, c in enumerate(costs)]

    def get_diversity(indiv1: TestIndividual, indiv2: TestIndividual):
        return dists[indiv1.idx, indiv2.idx]

    params = PopulationParams(
        use_numpy=use_numpy, min_pop_size=25, generation_size=n - 25 + 1
    )
    pop = GenericPopulation[TestIndividual](
        indivs[0], get_diversity, rng.integers, params=params
    )

    for indiv in indivs:
        pop.add(indiv)

    # Test that we haven't purged indeed
    assert_equal(n, len(pop))

    best = pop.get_best_found()
    assert_equal(costs.argmin(), best.idx)
    assert_almost_equal(costs.min(), best.cost())

    # Compute expected distance closest
    k = params.nb_close
    expected_div = np.partition(dists + np.eye(n) * 1e9, k - 1, axis=-1)[
        :, :k
    ].mean(-1)

    # Check that we have no duplicates which makes rank and biased fitness
    # computation unstable
    assert_(not has_duplicates(costs))
    assert_(not has_duplicates(expected_div))

    subpop = pop.feasible_subpopulation

    # Note: order of individuals can be arbitrary
    for i, indiv in enumerate(subpop):
        assert_almost_equal(
            expected_div[indiv.idx], subpop.avg_distance_closest(i)
        )

    cost_rank = get_rank(costs)
    div_rank = get_rank(-expected_div)

    nb_elite = min(params.nb_elite, n)
    div_weight = 1 - nb_elite / n

    expected_fitness = (cost_rank + div_weight * div_rank) / n

    # Note: order of individuals can be arbitrary
    for i, indiv in enumerate(subpop):
        assert_almost_equal(
            expected_fitness[indiv.idx], subpop.get_biased_fitness(i)
        )

    # Test that if we do a binary tournament many times, the ranking of the
    # sampled items should approximately equal the ranking of the fitness
    n_trials = n * 100
    freqs = np.bincount(
        [pop.get_binary_tournament().idx for _ in range(n_trials)]
    )
    rank_binary_tournament = get_rank(-freqs)  # Higher is better
    fitness_rank = get_rank(expected_fitness)  # Lower is better
    perc_rank_off = (rank_binary_tournament - fitness_rank) / n
    assert_(np.abs(perc_rank_off).mean() < 0.1)

    # Test that if we purge we purge the individual with wordt biased fitness
    # Note that if we have min_pop_size 1, generation size 1, then max size = 2
    # And we purge after we add the 3rd indiv back to 1 so we always purge at
    # least 2 indivs but these do not need to be the two with worst fitness
    # as the fitness gets updated after removing the first individual
    params = PopulationParams(
        use_numpy=use_numpy, min_pop_size=n - 1, generation_size=1
    )
    pop = GenericPopulation[TestIndividual](
        indivs[0], get_diversity, rng.integers, params=params
    )

    for indiv in indivs[:-1]:
        pop.add(indiv)

    assert_equal(len(pop), n - 1)
    # Now add last indiv which should trigger purge
    pop.add(indivs[-1])
    assert_equal(len(pop), n - 1)

    subpop = pop.feasible_subpopulation

    assert_(indivs[expected_fitness.argmax()] not in subpop)

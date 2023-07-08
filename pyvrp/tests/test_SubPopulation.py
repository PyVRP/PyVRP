import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal
from pytest import mark

from pyvrp import CostEvaluator, PopulationParams, Solution, XorShift128
from pyvrp._pyvrp import SubPopulation
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.tests.helpers import read


@mark.parametrize("nb_close", [5, 10, 25])
def test_avg_distance_closest_is_same_up_to_nb_close(nb_close: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=5)

    params = PopulationParams(
        min_pop_size=0, generation_size=250, nb_close=nb_close
    )

    subpop = SubPopulation(bpd, params)
    assert_equal(len(subpop), 0)

    for _ in range(nb_close):
        subpop.add(Solution.make_random(data, rng), cost_evaluator)

    # The first nb_close solutions all have each other in their "closest"
    # list. The averages only differ because each solution is themselves not
    # in their own list. So we would expect these values to all be pretty
    # similar.
    distances = np.array([item.avg_distance_closest() for item in subpop])
    assert_allclose(distances, distances.mean(), rtol=1 / len(subpop))
    assert_equal(len(subpop), nb_close)

    # Let's add a significantly larger set of solutions.
    for _ in range(250 - nb_close):
        subpop.add(Solution.make_random(data, rng), cost_evaluator)

    # Now the "closest" lists should differ quite a bit between solutions,
    # and the average distances should thus not all be the same any more.
    distances = np.array([item.avg_distance_closest() for item in subpop])
    assert_equal(len(subpop), params.max_pop_size)
    assert_(not np.allclose(distances, distances.mean(), rtol=1 / len(subpop)))


def test_avg_distance_closest_for_single_route_solutions():
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    params = PopulationParams(min_pop_size=0, nb_close=10)

    subpop = SubPopulation(bpd, params)
    assert_equal(len(subpop), 0)

    single_route = [client for client in range(1, data.num_clients + 1)]

    for offset in range(params.max_pop_size):
        # This is a single-route solution, but the route is continually shifted
        # (or rotated) around the depot.
        shifted_route = single_route[-offset:] + single_route[:-offset]
        shifted = Solution(data, [shifted_route])

        for item in subpop:
            # Every solution already in the subpopulation has exactly two
            # broken links with this new shifted solution, both around the
            # depot. So the average broken pairs distance is 2 / num_clients
            # for all of them.
            assert_equal(bpd(item.solution, shifted), 2 / data.num_clients)

        subpop.add(shifted, cost_evaluator)
        assert_equal(len(subpop), offset + 1)

        # Since the broken pairs distance is the same for all solutions, the
        # average distance amongst the closest solutions should also be the
        # same for all of them.
        distances = np.array([item.avg_distance_closest() for item in subpop])
        assert_allclose(distances, distances.mean())


def test_fitness_is_purely_based_on_cost_when_only_elites():
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=51)
    params = PopulationParams(nb_elite=25, min_pop_size=25)
    subpop = SubPopulation(bpd, params)

    for _ in range(params.min_pop_size):
        subpop.add(Solution.make_random(data, rng), cost_evaluator)

    # We need to call update_fitness before accessing the fitness
    subpop.update_fitness(cost_evaluator)

    # When all solutions are elite the diversity weight term drops out, and
    # fitness rankings are purely based on the cost ranking.
    cost = np.array(
        [cost_evaluator.penalised_cost(item.solution) for item in subpop]
    )
    by_cost = np.argsort(cost, kind="stable")

    rank = np.empty(len(subpop))
    rank[by_cost] = np.arange(len(subpop))

    expected_fitness = rank / (2 * len(subpop))
    actual_fitness = np.array([item.fitness for item in subpop])

    # The fitness terms should all be bounded to [0, 1], and the values should
    # agree with what we've computed above.
    assert_(((actual_fitness >= 0) & (actual_fitness <= 1)).all())
    assert_allclose(actual_fitness, expected_fitness)


def test_fitness_is_average_of_cost_and_diversity_when_no_elites():
    data = read("data/RC208.txt", "solomon", "dimacs")
    cost_evaluator = CostEvaluator(20, 6)
    rng = XorShift128(seed=52)
    params = PopulationParams(nb_elite=0, min_pop_size=25)
    subpop = SubPopulation(bpd, params)

    for _ in range(params.min_pop_size):
        subpop.add(Solution.make_random(data, rng), cost_evaluator)

    # We need to call update_fitness before accessing the fitness
    subpop.update_fitness(cost_evaluator)

    # When no solutions are elite, the fitness ranking is based on the mean
    # of the cost and diversity ranks.
    cost = np.array(
        [cost_evaluator.penalised_cost(item.solution) for item in subpop]
    )
    cost_rank = np.argsort(cost, kind="stable")

    diversity = np.array([item.avg_distance_closest() for item in subpop])
    div_rank = np.argsort(-diversity[cost_rank], kind="stable")

    ranks = np.empty((len(subpop), 2))
    ranks[cost_rank, 0] = np.arange(len(subpop))
    ranks[cost_rank[div_rank], 1] = np.arange(len(subpop))

    expected_fitness = ranks.sum(axis=1) / (2 * len(subpop))
    actual_fitness = np.array([item.fitness for item in subpop])

    # The fitness terms should all be bounded to [0, 1], and the values should
    # agree with what we've computed above.
    assert_(((actual_fitness >= 0) & (actual_fitness <= 1)).all())
    assert_allclose(actual_fitness, expected_fitness)

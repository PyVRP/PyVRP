import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal
from pytest import mark

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp._SubPopulation import PopulationParams, SubPopulation
from pyvrp.diversity import broken_pairs_distance
from pyvrp.tests.helpers import read


@mark.parametrize("nb_close", [5, 10, 25])
def test_avg_distance_closest_is_same_up_to_nb_close(nb_close: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.num_vehicles)
    rng = XorShift128(seed=5)

    params = PopulationParams(
        min_pop_size=0, generation_size=250, nb_close=nb_close
    )

    subpop = SubPopulation(data, broken_pairs_distance, params)
    assert_equal(len(subpop), 0)

    for _ in range(nb_close):
        subpop.add(Individual(data, pm, rng))

    # The first nb_close individuals all have each other in their "closest"
    # list. The averages only differ because each individual is themselves not
    # in their own list. So we would expect these values to all be pretty
    # similar.
    distances = np.array([item.avg_distance_closest() for item in subpop])
    assert_allclose(distances, distances.mean(), rtol=1 / len(subpop))
    assert_equal(len(subpop), nb_close)

    # Let's add a significantly larger set of individuals.
    for _ in range(250 - nb_close):
        subpop.add(Individual(data, pm, rng))

    # Now the "closest" lists should differ significantly between individuals,
    # and should thus not all be approximately the same.
    distances = np.array([item.avg_distance_closest() for item in subpop])
    assert_equal(len(subpop), params.max_pop_size)
    assert_(not np.allclose(distances, distances.mean(), rtol=1 / len(subpop)))

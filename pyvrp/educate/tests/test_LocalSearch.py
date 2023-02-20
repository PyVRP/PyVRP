from numpy.testing import assert_raises

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.educate import LocalSearch
from pyvrp.tests.helpers import read


def test_local_search_raises_when_there_are_no_operators():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    ls = LocalSearch(data, pm, rng)
    individual = Individual(data, pm, rng)

    with assert_raises(RuntimeError):
        ls.search(individual)

    with assert_raises(RuntimeError):
        ls.intensify(individual)


def test_local_search_raises_when_neighbourhood_structure_is_empty():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    ls = LocalSearch(data, pm, rng)
    ls.set_neighbours([[] for _ in range(data.num_clients + 1)])

    individual = Individual(data, pm, rng)

    with assert_raises(RuntimeError):
        ls.search(individual)

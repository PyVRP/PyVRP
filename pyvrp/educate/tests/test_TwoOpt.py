from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.educate import (
    LocalSearch,
    NeighbourhoodParams,
    TwoOpt,
    compute_neighbours,
)
from pyvrp.tests.helpers import read


def test_OkSmall_instance():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, params=nb_params))
    ls.add_node_operator(TwoOpt(data, pm))

    individual = Individual(data, pm, [[1, 2, 3, 4]])
    improved_individual = ls.search(individual)

    # The new solution should strictly improve on our original solution.
    assert_equal(improved_individual.num_routes(), 2)
    assert_(improved_individual.cost() < individual.cost())

    # First improving (U, V) node pair is (1, 3), which results in the route
    # [1, 3, 2, 4]. The second improving node pair involves the depot of an
    # empty route: (1, 0). This results in routes [3, 2, 4] and [1].
    assert_equal(improved_individual.get_routes(), [[3, 2, 4], [1], []])


@mark.parametrize("seed", [2643, 2742, 2941, 3457, 4299, 4497, 6178, 6434])
def test_RC208_instance(seed: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=seed)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, params=nb_params))
    ls.add_node_operator(TwoOpt(data, pm))

    single_route = list(range(1, data.num_clients + 1))
    individual = Individual(data, pm, [single_route])
    improved_individual = ls.search(individual)

    # The new solution should strictly improve on our original solution.
    assert_(improved_individual.cost() < individual.cost())

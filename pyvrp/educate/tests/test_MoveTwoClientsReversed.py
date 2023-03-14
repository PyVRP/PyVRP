from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.educate import (
    LocalSearch,
    MoveTwoClientsReversed,
    NeighbourhoodParams,
    compute_neighbours,
)
from pyvrp.tests.helpers import read


def test_single_route_OkSmall():
    """
    This test checks that MoveTwoClientsReversed properly solves the small
    instance where we know what is going on.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager()
    rng = XorShift128(seed=42)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(MoveTwoClientsReversed(data))

    individual = Individual(data, pm, [[1, 4, 2, 3]])
    improved_individual = ls.search(individual)

    # The new solution should strictly improve on our original solution.
    assert_equal(improved_individual.num_routes(), 1)
    assert_(improved_individual.cost() < individual.cost())

    # (2, 3) was inserted after 1 as 1 -> 3 -> 2 -> 4. Then (1, 3) got inserted
    # after 2 as 2 -> 3 -> 1 -> 4.
    assert_equal(improved_individual.get_routes(), [[2, 3, 1, 4], [], []])

    # These two-route solutions can all be created by MoveTwoClientsReversed
    # from the returned solution. So they must have a cost that's at best equal
    # to the returned solution's cost.
    for routes in ([[3, 2], [1, 4]], [[2, 3], [4, 1]], [[2, 4], [1, 3]]):
        other = Individual(data, pm, routes)
        assert_(improved_individual.cost() <= other.cost())


@mark.parametrize("seed", [2643, 2742, 2941, 3457, 4299, 4497, 6178, 6434])
def test_RC208_instance(seed: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager()
    rng = XorShift128(seed=seed)

    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, compute_neighbours(data, nb_params))
    ls.add_node_operator(MoveTwoClientsReversed(data))

    single_route = list(range(1, data.num_clients + 1))
    individual = Individual(data, pm, [single_route])
    improved_individual = ls.search(individual)

    # The new solution should strictly improve on our original solution.
    assert_(improved_individual.cost() < individual.cost())

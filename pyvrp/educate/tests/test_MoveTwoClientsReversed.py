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
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    ls = LocalSearch(data, pm, rng)
    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls.set_neighbours(compute_neighbours(data, nb_params))

    op = MoveTwoClientsReversed(data, pm)
    ls.add_node_operator(op)

    individual = Individual(data, pm, [[1, 4, 2, 3]])
    copy = Individual(individual)

    ls.search(individual)

    # The new solution should strictly improve on our original solution.
    assert_equal(individual.num_routes(), 1)
    assert_(individual.cost() < copy.cost())

    # (2, 3) was inserted after the depot as (3, 2)
    assert_equal(individual.get_routes(), [[3, 2, 1, 4], [], []])

    # These two-route solutions can all be created by MoveTwoClientsReversed
    # from the returned solution. So they must have a cost that's at best equal
    # to the returned solution's cost.
    for routes in ([[3, 2], [4, 1]], [[2, 3], [1, 4]], [[3, 4], [1, 2]]):
        other = Individual(data, pm, routes)
        assert_(individual.cost() <= other.cost())


@mark.parametrize("seed", [2643, 2742, 2941, 3457, 4299, 4497, 6178, 6434])
def test_RC208_instance(seed: int):
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=seed)

    ls = LocalSearch(data, pm, rng)
    nb_params = NeighbourhoodParams(nb_granular=data.num_clients)
    ls.set_neighbours(compute_neighbours(data, nb_params))

    op = MoveTwoClientsReversed(data, pm)
    ls.add_node_operator(op)

    single_route = list(range(1, data.num_clients + 1))
    individual = Individual(data, pm, [single_route])
    copy = Individual(individual)

    ls.search(individual)

    # The new solution should strictly improve on our original solution.
    assert_(individual.cost() < copy.cost())

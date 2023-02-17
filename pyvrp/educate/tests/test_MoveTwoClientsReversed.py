from numpy.testing import assert_, assert_equal

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.educate import (
    LocalSearch,
    LocalSearchParams,
    MoveTwoClientsReversed,
)
from pyvrp.tests.helpers import read


def test_single_route_OkSmall():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    params = LocalSearchParams(nb_granular=data.num_clients)
    ls = LocalSearch(data, pm, rng, params)
    op = MoveTwoClientsReversed(data, pm)
    ls.add_node_operator(op)

    individual = Individual(data, pm, [[1, 4, 2, 3]])
    copy = Individual(individual)

    ls.search(individual)

    # The new solution should strictly improve on our original solution.
    assert_equal(individual.num_routes(), 1)
    assert_(individual.cost() < copy.cost())

    print(individual.get_routes())

    # (2, 3) was inserted after the depot as (3, 2)
    assert_equal(individual.get_routes(), [[3, 2, 1, 4], [], []])

    # These alternative, two-route solution are all reachable from the returned
    # solution. And they can be created by MoveTwoClientsReversed. So they must
    # have a cost that's at best equal to the returned solution's cost.
    for routes in ([[3, 2], [4, 1]], [[2, 3], [1, 4]], [[3, 4], [1, 2]]):
        other = Individual(data, pm, routes)
        assert_(individual.cost() <= other.cost())

from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.educate import Exchange11, Exchange22, Exchange33, LocalSearch
from pyvrp.tests.helpers import read


@mark.parametrize("operator", [Exchange11, Exchange22, Exchange33])
def test_swap_single_route_stays_single_route(operator):
    """
    Swap operators on a single route can only move within the same route, so
    they can never find a solution that has more than one route.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    ls = LocalSearch(data, pm, rng)
    op = operator(data, pm)
    ls.add_node_operator(op)

    single_route = list(range(1, data.num_clients + 1))
    individual = Individual(data, pm, [single_route])
    curr_cost = individual.cost()

    ls.search(individual)

    # The new solution should strictly improve on our original solution.
    assert_equal(individual.num_routes(), 1)
    assert_(curr_cost > individual.cost())

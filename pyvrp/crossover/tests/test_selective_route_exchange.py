from numpy.testing import assert_equal

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.tests.helpers import read


def test_srex_identical_individuals():
    """
    Tests if crossing two identical individuals yields an identical offspring.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)  # nMovedroutes = 2

    indiv1 = Individual(data, pm, [[1, 3], [4], [2]])
    indiv2 = Individual(data, pm, [[1, 3], [4], [2]])
    parents = (indiv1, indiv2)

    offspring = srex(parents, data, pm, rng)

    assert_equal(offspring.get_routes(), indiv1.get_routes())
    assert_equal(offspring.get_routes(), indiv2.get_routes())


def test_srex_offspring_is_second_parent_when_all_routes_moved():
    """
    Tests if all routes are moved during crossover, then SREX produces an
    an offspring that is identical to the second parent.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=4)  # results in nMovedRoutes = 3

    indiv1 = Individual(data, pm, [[1], [2], [3, 4]])
    indiv2 = Individual(data, pm, [[1, 3], [4], [2]])
    parents = (indiv1, indiv2)

    offspring = srex(parents, data, pm, rng)

    assert_equal(offspring.get_routes(), indiv2.get_routes())


def test_srex_identical_single_route_individuals():
    """
    Tests that crossing two identical individuals with a single route
    yields the same route.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)  # results in nMovedRoutes = 1

    indiv1 = Individual(data, pm, [[1, 2, 3, 4], [], []])
    indiv2 = Individual(data, pm, [[], [], [1, 2, 3, 4]])
    parents = (indiv1, indiv2)

    # Indiv2's route [2, 3, 4] will be moved. This results in two
    # candidate offspring individuals: [[1], [2, 3, 4]] and
    offspring = srex(parents, data, pm, rng)

    assert_equal(offspring.get_routes(), indiv2.get_routes())


def test_srex_move_one_route():
    """
    Tests that crossing two identical individuals with a single route
    yields the same route.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=0)  # results in startA = 1 and nMovedRoutes = 1

    indiv1 = Individual(data, pm, [[1], [2, 3], [4]])
    indiv2 = Individual(data, pm, [[4], [3, 2], [1]])
    parents = (indiv1, indiv2)

    # The search for which routes to move starts at the middle routes. Moving
    # right or left does not lead to route replacements that have less
    # differences, so the middle routes will be moved.
    offspring = srex(parents, data, pm, rng)

    assert_equal(offspring.get_routes(), [[1], [3, 2], [4]])

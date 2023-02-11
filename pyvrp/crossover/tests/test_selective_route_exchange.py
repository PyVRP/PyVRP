from numpy.testing import assert_equal

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.tests.helpers import read


def test_srex_move_all_routes():
    """
    Tests if all routes are moved during crossover, then SREX produces
    an offspring that is identical to the second parent.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=4)  # nMovedRoutes = 3

    indiv1 = Individual(data, pm, [[1], [2], [3, 4]])
    indiv2 = Individual(data, pm, [[1, 2], [3], [4]])
    parents = (indiv1, indiv2)

    offspring = srex(parents, data, pm, rng)

    assert_equal(offspring.get_routes(), indiv2.get_routes())


def test_srex_move_start_indinces():
    """
    Tests the case where the initial start indices are changed.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=2)  # startA = 0 and nMovedRoutes = 1

    indiv1 = Individual(data, pm, [[1, 2, 3], [4], []])
    indiv2 = Individual(data, pm, [[1, 2, 4], [3], []])
    parents = (indiv1, indiv2)

    # The start indices at initialization are startA = 0 and startB = 0.
    # The difference for A to move left (= right) is -1. The difference for B
    # to move left (= right) is 0. The new indices become startA = 1 and
    # startB = 0. There are no improving moves in this position since the
    # difference for A is 1 and difference for B is 1.
    # So, indiv1's route [4] will be replaced by indiv2's route [1, 2, 4].
    # This results in two candidate offspring, [[3], [1, 2, 4]] with cost
    # 10195, and [[1, 2, 3], [4]] with cost 31029. The first candidate is
    # returned, because it has the lowest cost
    offspring = srex(parents, data, pm, rng)

    assert_equal(offspring.get_routes(), [[3], [1, 2, 4], []])


def test_srex_greedy_repair():
    """
    Tests the case where greedy repair is used during SREX crossover.
    """
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=2)  # startA = 0 and nMovedRoutes = 1

    indiv1 = Individual(data, pm, [[1, 2], [3, 4], []])
    indiv2 = Individual(data, pm, [[2, 3], [4, 1], []])
    parents = (indiv1, indiv2)

    # The start indices do not change because there are no improving moves.
    # So, indiv1's route [1, 2] will be replaced by indiv2's route [2, 3].
    # This results in two incomplete offspring, [[2, 3], [4]] and [[2], [3, 4]]
    # which are both repaired by greedy repair. After repair, we obtain the
    # offspring [[2, 3, 1], [4]] with cost 8735, and [[1, 2], [3, 4]] with
    # cost 9725.
    offspring = srex(parents, data, pm, rng)

    assert_equal(offspring.get_routes(), [[2, 3, 1], [4], []])

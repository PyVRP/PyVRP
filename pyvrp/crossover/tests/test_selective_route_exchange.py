from numpy.testing import assert_equal, assert_raises
from pytest import mark

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.crossover._selective_route_exchange import (
    selective_route_exchange as cpp_srex,
)
from pyvrp.tests.helpers import read


def test_same_parents_same_offspring():
    """
    Tests that SREX produces identical offspring when both parents are the
    same.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = PenaltyManager().get_cost_evaluator()
    rng = XorShift128(seed=42)

    individual = Individual(data, [[1, 2], [3, 4]])
    offspring = srex((individual, individual), data, cost_evaluator, rng)

    assert_equal(offspring, individual)


@mark.parametrize(
    "idx1, idx2, num_moved_routes",
    [
        (10, 0, 1),  # idx1 >= # routes first
        (0, 10, 1),  # idx2 >= # routes second
        (0, 0, 0),  # num_moved_routes < 1
        (0, 0, 2),  # num_moved_routes > min(# routes first, # routes second)
    ],
)
def test_raise_invalid_arguments(idx1, idx2, num_moved_routes):
    data = read("data/OkSmall.txt")
    cost_evaluator = PenaltyManager().get_cost_evaluator()

    indiv1 = Individual(data, [[1], [2], [3, 4]])
    indiv2 = Individual(data, [[1, 2, 3, 4]])

    with assert_raises(ValueError):
        cpp_srex(
            (indiv1, indiv2),
            data,
            cost_evaluator,
            (idx1, idx2),
            num_moved_routes,
        )


def test_srex_move_all_routes():
    """
    Tests if SREX produces an offspring that is identical to the second parent
    when all routes are replaced during crossover.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = PenaltyManager().get_cost_evaluator()

    indiv1 = Individual(data, [[1], [2], [3, 4]])
    indiv2 = Individual(data, [[1, 2], [3], [4]])
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 3)

    assert_equal(offspring, indiv2)


def test_srex_greedy_repair():
    """
    Tests the case where greedy repair is used during SREX crossover.
    """
    data = read("data/OkSmallGreedyRepair.txt")
    cost_evaluator = PenaltyManager().get_cost_evaluator()

    indiv1 = Individual(data, [[1, 2], [3, 4]])
    indiv2 = Individual(data, [[2, 3], [4, 1]])

    # The start indices do not change because there are no improving moves.
    # So, indiv1's route [1, 2] will be replaced by indiv2's route [2, 3].
    # This results in two incomplete offspring [[2, 3], [4]] and [[2], [3, 4]],
    # which are both repaired using greedy repair. After repair, we obtain the
    # offspring [[2, 3, 1], [4]] with cost 8735, and [[1, 2], [3, 4]] with
    # cost 9725. The first one is returned since it has the lowest cost.
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)

    assert_equal(offspring.get_routes(), [[2, 3, 1], [4], []])


def test_srex_changed_start_indices():
    """
    Tests the case where the initial start indices are changed in SREX.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = PenaltyManager().get_cost_evaluator()

    indiv1 = Individual(data, [[1, 2, 3], [4]])
    indiv2 = Individual(data, [[1, 2, 4], [3]])

    # The difference for A to move left (= right) is -1. The difference for B
    # to move left (= right) is 1. The new indices become idx1 = 1 and
    # idx2 = 0. There are no improving moves in this position since the
    # difference for A to move is 1 and difference for B to move is 1.
    # So, indiv1's route [4] will be replaced by indiv2's route [1, 2, 4].
    # This results in two candidate offspring, [[3], [1, 2, 4]] with cost
    # 10195, and [[1, 2, 3], [4]] with cost 31029. The first candidate is
    # returned since it has the lowest cost.
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)

    assert_equal(offspring.get_routes(), [[3], [1, 2, 4], []])


def test_srex_a_left_move():
    """
    Tests the case where the initial start indices are changed by moving the
    A index to the left.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = PenaltyManager().get_cost_evaluator()

    indiv1 = Individual(data, [[1, 3], [2], [4]])
    indiv2 = Individual(data, [[4, 1], [2], [3]])

    # We describe the A-left case here in detail. The tests below for A-right,
    # B-left and B-right can be worked out similarly: note that we only change
    # the ordering of the routes.
    #
    # Initial start indices (indicated by **)
    # *[1, 3]* [2] [4]
    # *[4, 1]* [2] [3]
    #
    # Differences
    # A-left:   0 - 1 = -1
    # A-right:  1 - 1 =  0
    # B-left:   1 - 1 =  0
    # B-right:  1 - 0 =  1
    #
    # New start indices
    # [1, 3] [2] *[4]*
    # *[4, 1]* [2] [3]
    #
    # Differences
    # A-left:   1 - 0 = 1
    # A-right:  1 - 0 = 1
    # B-left:   1 - 0 = 1
    # B-right:  1 - 0 = 1
    #
    # No more improving moves.
    #
    # Candidate offspring
    # [1, 3] [2] [4] - cost: 24416
    # [3] [2] [4, 1] - cost: 12699 <-- selected as new offspring
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)

    assert_equal(offspring.get_routes(), [[3], [2], [4, 1]])


def test_srex_a_right_move():
    """
    Tests the case where the initial start indices are changed by moving to
    A index to the right.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = PenaltyManager().get_cost_evaluator()

    indiv1 = Individual(data, [[1, 3], [4], [2]])
    indiv2 = Individual(data, [[4, 1], [2], [3]])
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)

    assert_equal(offspring.get_routes(), [[3], [4, 1], [2]])


def test_srex_b_left_move():
    """
    Tests the case where the initial start indices are changed by moving the
    B index to the left.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = PenaltyManager().get_cost_evaluator()

    indiv1 = Individual(data, [[4], [2], [1, 3]])
    indiv2 = Individual(data, [[3], [2], [4, 1]])
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)

    assert_equal(offspring.get_routes(), [[4, 1], [2], [3]])


def test_srex_b_right_move():
    """
    Tests the case where the initial start indices are changed by moving the
    B index to the right.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = PenaltyManager().get_cost_evaluator()

    indiv1 = Individual(data, [[4], [2], [1, 3]])
    indiv2 = Individual(data, [[3], [4, 1], [2]])
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)

    assert_equal(offspring.get_routes(), [[4, 1], [2], [3]])

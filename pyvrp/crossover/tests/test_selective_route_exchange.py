import itertools

from numpy.testing import assert_equal, assert_raises
from pytest import mark

from pyvrp import CostEvaluator, Individual, XorShift128
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
    cost_evaluator = CostEvaluator(20, 6)
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
    cost_evaluator = CostEvaluator(20, 6)

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
    cost_evaluator = CostEvaluator(20, 6)

    indiv1 = Individual(data, [[1], [2], [3, 4]])
    indiv2 = Individual(data, [[1, 2], [3], [4]])
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 3)

    # Note: result will be permuted but equality is invariant to that
    assert_equal(offspring, indiv2)


def test_srex_sorts_routes():
    """
    Tests if SREX sorts the input before applying the operator.

    Internally, SREX first sorts the given routes by angle w.r.t. the depot.
    Since that always results in the same route order for the same set of
    routes, SREX should be invariant to route permutations and always produce
    the exact same offspring.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)

    routes1 = [[1], [2], [3, 4]]
    routes2 = [[1, 2], [3], [4]]
    indiv1 = Individual(data, routes1)
    indiv2 = Individual(data, routes2)
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)

    for permuted_routes1 in itertools.permutations(routes1):
        for permuted_routes2 in itertools.permutations(routes2):
            indiv1 = Individual(data, permuted_routes1)
            indiv2 = Individual(data, permuted_routes2)
            offspring_permuted = cpp_srex(
                (indiv1, indiv2), data, cost_evaluator, (0, 0), 1
            )
            assert_equal(offspring_permuted, offspring)


def test_srex_greedy_repair():
    """
    Tests the case where greedy repair is used during SREX crossover.
    """
    data = read("data/OkSmallGreedyRepair.txt")
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them
    indiv1 = Individual(data, [[3, 4], [1, 2]])
    indiv2 = Individual(data, [[2, 3], [4, 1]])

    # The start indices do not change because there are no improving moves.
    # So, indiv1's route [3, 4] will be replaced by indiv2's route [2, 3].
    # This results in two incomplete offspring [[2, 3], [1]] and [[3], [1, 2]],
    # which are both repaired using greedy repair. After repair, we obtain the
    # offspring [[2, 3, 4], [1]] with cost 8188, and [[3, 4], [1, 2]] with
    # cost 9725. The first one is returned since it has the lowest cost.
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)
    expected = Individual(data, [[2, 3, 4], [1]])
    assert_equal(offspring, expected)


def test_srex_changed_start_indices():
    """
    Tests the case where the initial start indices are changed in SREX.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them
    indiv1 = Individual(data, [[4], [1, 2, 3]])
    indiv2 = Individual(data, [[3], [1, 2, 4]])

    # We will start with idx1 = 1 and idx2 = 1 (1 for both indivs)
    # The difference for A to move left (= right) is -1. The difference for B
    # to move left (= right) is 1. The new indices become idx1 = 0 and
    # idx2 = 1. There are no improving moves in this position since the
    # difference for A to move is 1 and difference for B to move is 1.
    # So, indiv1's route [4] will be replaced by indiv2's route [1, 2, 4].
    # This results in two candidate offspring, [[1, 2, 4], [3]] with cost
    # 10195, and [[4], [1, 2, 3]] with cost 31029. The first candidate is
    # returned since it has the lowest cost.
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (1, 1), 1)
    expected = Individual(data, [[1, 2, 4], [3]])
    assert_equal(offspring, expected)


def test_srex_a_right_move():
    """
    Tests the case where the initial start indices are changed by moving the
    A index to the right.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them
    indiv1 = Individual(data, [[4], [2], [1, 3]])
    indiv2 = Individual(data, [[3], [2], [4, 1]])

    # We describe the A-right case here in detail. The tests below for A-left,
    # B-left and B-right can be worked out similarly. Note that the test cases
    # are somewhat contrived since the input routes must be sorted by angle.
    #
    # Initial start indices (indicated by **) A\B = {1,3} \ {1, 4} = {3}, # = 1
    # [4] [2] *[1, 3]*
    # [3] [2] *[4, 1]*
    #
    # Differences
    # A-left:   1 - 1 =  0
    # A-right:  0 - 1 = -1
    # B-left:   1 - 0 =  1
    # B-right:  1 - 1 =  0
    #
    # New start indices A\B = {4} \ {1, 4} = {} # = 0
    # *[4]* [2] [1, 3]
    # [3] [2] *[4, 1]*
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
    # [4, 1] [2] [3] - cost: 12699 <-- selected as new offspring
    # [4] [2] [1, 3] - cost: 24416
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (2, 2), 1)
    expected = Individual(data, [[4, 1], [2], [3]])
    assert_equal(offspring, expected)


def test_srex_a_left_move():
    """
    Tests the case where the initial start indices are changed by moving to
    A index to the left.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them
    indiv1 = Individual(data, [[4], [2], [1, 3]])
    indiv2 = Individual(data, [[3], [4], [2, 1]])
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (2, 2), 1)
    expected = Individual(data, [[4], [2, 1], [3]])
    assert_equal(offspring, expected)


def test_srex_b_left_move():
    """
    Tests the case where the initial start indices are changed by moving the
    B index to the left.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them
    indiv1 = Individual(data, [[4], [2], [1, 3]])
    indiv2 = Individual(data, [[3], [2], [4, 1]])
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)
    expected = Individual(data, [[4, 1], [2], [3]])
    assert_equal(offspring, expected)


def test_srex_b_right_move():
    """
    Tests the case where the initial start indices are changed by moving the
    B index to the right.
    """
    data = read("data/OkSmall.txt")
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them
    indiv1 = Individual(data, [[4], [2], [1, 3]])
    indiv2 = Individual(data, [[3], [4], [2, 1]])
    offspring = cpp_srex((indiv1, indiv2), data, cost_evaluator, (0, 0), 1)
    expected = Individual(data, [[4], [2], [1, 3]])
    assert_equal(offspring, expected)

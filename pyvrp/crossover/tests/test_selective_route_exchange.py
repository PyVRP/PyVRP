import itertools

from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import (
    CostEvaluator,
    RandomNumberGenerator,
    Route,
    Solution,
    VehicleType,
)
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.crossover._crossover import selective_route_exchange as cpp_srex
from pyvrp.tests.helpers import make_heterogeneous


def test_same_parents_same_offspring(ok_small):
    """
    Tests that SREX produces identical offspring when both parents are the
    same.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    solution = Solution(ok_small, [[1, 2], [3, 4]])
    offspring = srex((solution, solution), ok_small, cost_evaluator, rng)

    assert_equal(offspring, solution)


def test_srex_empty_solution(prize_collecting):
    """
    Tests that SREX returns the other parent when one of the solutions is
    empty. This can occur during prize collecting, and in that case there are
    no routes to exchange.
    """
    cost_evaluator = CostEvaluator(20, 6)
    rng = RandomNumberGenerator(seed=42)

    empty = Solution(prize_collecting, [])
    nonempty = Solution(prize_collecting, [[1, 2, 3, 4]])

    # If both parents are empty the returned offspring must also be empty.
    offspring = srex((empty, empty), prize_collecting, cost_evaluator, rng)
    assert_equal(offspring, empty)

    # If one of the two parents is empty but the other is not, the returned
    # solution is the nonempty parent.
    for parents in [(empty, nonempty), (nonempty, empty)]:
        offspring = srex(parents, prize_collecting, cost_evaluator, rng)
        assert_equal(offspring, nonempty)


@mark.parametrize(
    ("idx1", "idx2", "num_moved_routes"),
    [
        (10, 0, 1),  # idx1 >= # routes first
        (0, 10, 1),  # idx2 >= # routes second
        (0, 0, 0),  # num_moved_routes < 1
        (0, 0, 2),  # num_moved_routes > min(# routes first, # routes second)
    ],
)
def test_raise_invalid_arguments(ok_small, idx1, idx2, num_moved_routes):
    """
    Tests that SREX raises when some of the arguments are invalid.
    """
    cost_evaluator = CostEvaluator(20, 6)

    sol1 = Solution(ok_small, [[1], [2], [3, 4]])
    sol2 = Solution(ok_small, [[1, 2, 3, 4]])

    with assert_raises(ValueError):
        cpp_srex(
            (sol1, sol2),
            ok_small,
            cost_evaluator,
            (idx1, idx2),
            num_moved_routes,
        )


def test_srex_move_all_routes(ok_small):
    """
    Tests if SREX produces an offspring that is identical to the second parent
    when all routes are replaced during crossover.
    """
    cost_evaluator = CostEvaluator(20, 6)

    sol1 = Solution(ok_small, [[1], [2], [3, 4]])
    sol2 = Solution(ok_small, [[1, 2], [3], [4]])
    offspring = cpp_srex((sol1, sol2), ok_small, cost_evaluator, (0, 0), 3)

    # Note: result will be permuted but equality is invariant to that.
    assert_equal(offspring, sol2)


def test_srex_sorts_routes(ok_small):
    """
    Tests if SREX sorts the input before applying the operator.

    Internally, SREX first sorts the given routes by angle w.r.t. the depot.
    Since that always results in the same route order for the same set of
    routes, SREX should be invariant to route permutations and always produce
    the exact same offspring.
    """
    cost_evaluator = CostEvaluator(20, 6)

    routes1 = [[1], [2], [3, 4]]
    routes2 = [[1, 2], [3], [4]]
    sol1 = Solution(ok_small, routes1)
    sol2 = Solution(ok_small, routes2)
    offspring = cpp_srex((sol1, sol2), ok_small, cost_evaluator, (0, 0), 1)

    for permuted_routes1 in itertools.permutations(routes1):
        for permuted_routes2 in itertools.permutations(routes2):
            sol1 = Solution(ok_small, permuted_routes1)
            sol2 = Solution(ok_small, permuted_routes2)
            permuted_offspring = cpp_srex(
                (sol1, sol2), ok_small, cost_evaluator, (0, 0), 1
            )
            assert_equal(permuted_offspring, offspring)


def test_srex_does_not_reinsert_unplanned_clients(ok_small):
    """
    Tests the case where the routes are not perfectly complementary. In that
    case, some clients will no longer in the solution after crossover: SREX
    does not reinsert those, but instead leaves that to the search method.
    """
    cost_evaluator = CostEvaluator(20, 6)

    # Note that when routes are exchanged, some customers will necessarily be
    # removed from the solution because the routes are overlapping. The
    # offspring are thus incomplete.
    sol1 = Solution(ok_small, [[3, 4], [1, 2]])
    sol2 = Solution(ok_small, [[2, 3], [4, 1]])

    # The start indices do not change because there are no improving moves.
    # So, sol1's route [3, 4] will be replaced by sol2's route [2, 3].
    # This results in two incomplete offspring [[2, 3], [1]] and [[3], [1, 2]],
    # the first with penalised cost 7898, the second with cost 9495. Thus, the
    # first one is returned.
    offspring = cpp_srex((sol1, sol2), ok_small, cost_evaluator, (0, 0), 1)
    expected = Solution(ok_small, [[2, 3], [1]])
    assert_equal(offspring, expected)
    assert_(not expected.is_complete())


def test_srex_changed_start_indices(ok_small):
    """
    Tests the case where the initial start indices are changed in SREX.
    """
    cost_evaluator = CostEvaluator(20, 6)

    sol1 = Solution(ok_small, [[4], [1, 2, 3]])
    sol2 = Solution(ok_small, [[3], [1, 2, 4]])

    # We will start with idx1 = 1 and idx2 = 1 (1 for both solutions)
    # The difference for A to move left (= right) is -1. The difference for B
    # to move left (= right) is 1. The new indices become idx1 = 0 and
    # idx2 = 1. There are no improving moves in this position since the
    # difference for A to move is 1 and difference for B to move is 1.
    # So, sol1's route [4] will be replaced by sol2's route [1, 2, 4].
    # This results in two candidate offspring, [[1, 2, 4], [3]] with cost
    # 10195, and [[4], [1, 2, 3]] with cost 31029. The first candidate is
    # returned since it has the lowest cost.
    offspring = cpp_srex((sol1, sol2), ok_small, cost_evaluator, (1, 1), 1)
    expected = Solution(ok_small, [[1, 2, 4], [3]])
    assert_equal(offspring, expected)


def test_srex_heterogeneous_changed_start_indices(ok_small):
    """
    Tests the case where the initial start indices are changed in SREX for
    heterogeneous routes.
    """
    data = make_heterogeneous(
        ok_small, [VehicleType(10, 2), VehicleType(20, 1)]
    )
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them
    sol1 = Solution(data, [[4], [1, 2, 3]])
    sol1h = Solution(data, [Route(data, [4], 0), Route(data, [1, 2, 3], 1)])
    sol2 = Solution(data, [[3], [1, 2, 4]])
    sol2h = Solution(data, [Route(data, [3], 0), Route(data, [1, 2, 4], 1)])

    # We will start with idx1 = 1 and idx2 = 1 (1 for both sols)
    # Note that the indices relate to the indices of the non-empty routes!
    # The difference for A to move left (= right) is -1. The difference for B
    # to move left (= right) is 1. The new indices become idx1 = 0 and
    # idx2 = 1. There are no improving moves in this position since the
    # difference for A to move is 1 and difference for B to move is 1.
    # So, sol1's route [4] will be replaced by sol2's route [1, 2, 4].
    # This results in two candidate offspring, [[1, 2, 4], [3]] with cost
    # 10195, and [[4], [1, 2, 3]] with cost 31029. The first candidate is
    # returned since it has the lowest cost.

    offspring = cpp_srex((sol1, sol2), data, cost_evaluator, (1, 1), 1)
    expected = [Route(data, [1, 2, 4], 0), Route(data, [3], 0)]
    assert_equal(offspring.get_routes(), expected)

    offspring = cpp_srex((sol1, sol2h), data, cost_evaluator, (1, 1), 1)
    expected = [Route(data, [1, 2, 4], 0), Route(data, [3], 0)]
    assert_equal(offspring.get_routes(), expected)

    offspring = cpp_srex((sol1h, sol2), data, cost_evaluator, (1, 1), 1)
    expected = [Route(data, [1, 2, 4], 0), Route(data, [3], 1)]
    assert_equal(offspring.get_routes(), expected)

    offspring = cpp_srex((sol1h, sol2h), data, cost_evaluator, (1, 1), 1)
    expected = [Route(data, [1, 2, 4], 0), Route(data, [3], 1)]


def test_srex_a_right_move(ok_small):
    """
    Tests the case where the initial start indices are changed by moving the
    A index to the right.
    """
    cost_evaluator = CostEvaluator(20, 6)

    sol1 = Solution(ok_small, [[4], [2], [1, 3]])
    sol2 = Solution(ok_small, [[3], [2], [4, 1]])

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
    offspring = cpp_srex((sol1, sol2), ok_small, cost_evaluator, (2, 2), 1)
    expected = Solution(ok_small, [[4, 1], [2], [3]])
    assert_equal(offspring, expected)


def test_srex_a_left_move(ok_small):
    """
    Tests the case where the initial start indices are changed by moving to
    A index to the left.
    """
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them.
    sol1 = Solution(ok_small, [[2], [1, 3], [4]])
    sol2 = Solution(ok_small, [[3], [2, 1], [4]])

    offspring = cpp_srex((sol1, sol2), ok_small, cost_evaluator, (2, 2), 1)
    expected = Solution(ok_small, [[4], [2, 1], [3]])
    assert_equal(offspring, expected)


def test_srex_b_left_move(ok_small):
    """
    Tests the case where the initial start indices are changed by moving the
    B index to the left.
    """
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them.
    sol1 = Solution(ok_small, [[2], [1, 3], [4]])
    sol2 = Solution(ok_small, [[3], [4, 1], [2]])
    offspring = cpp_srex((sol1, sol2), ok_small, cost_evaluator, (1, 2), 1)
    expected = Solution(ok_small, [[4, 1], [2], [3]])
    assert_equal(offspring, expected)


def test_srex_b_right_move(ok_small):
    """
    Tests the case where the initial start indices are changed by moving the
    B index to the right.
    """
    cost_evaluator = CostEvaluator(20, 6)

    # We create the routes sorted by angle such that SREX sorting doesn't
    # affect them.
    sol1 = Solution(ok_small, [[2], [1, 3], [4]])
    sol2 = Solution(ok_small, [[3], [2, 1], [4]])

    offspring = cpp_srex((sol1, sol2), ok_small, cost_evaluator, (2, 1), 1)
    expected = Solution(ok_small, [[4], [2], [1, 3]])
    assert_equal(offspring, expected)

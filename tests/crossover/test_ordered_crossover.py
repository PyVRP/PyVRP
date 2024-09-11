from numpy.testing import assert_equal, assert_raises

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution, VehicleType
from pyvrp.crossover import ordered_crossover as ox
from pyvrp.crossover._crossover import ordered_crossover as cpp_ox


def test_raises_when_not_tsp(ok_small):
    """
    Tests that the ordered crossover (OX) operator raises when used on a data
    instance that is not a TSP.
    """
    cost_eval = CostEvaluator(20, 6, 0)
    rng = RandomNumberGenerator(seed=42)
    sol = Solution(ok_small, [[1, 2], [3, 4]])

    # This is not a TSP, and thus OX should raise
    assert_equal(ok_small.num_vehicles, 3)
    with assert_raises(ValueError):
        ox((sol, sol), ok_small, cost_eval, rng)


def test_edge_cases_return_parents(pr107):
    """
    Tests that OX returns the first or second parent solution in two specific
    cases.
    """
    rng = RandomNumberGenerator(seed=42)

    sol1 = Solution.make_random(pr107, rng)
    sol2 = Solution.make_random(pr107, rng)

    # When the indices cover the whole range of clients, all clients from the
    # first route are copied in and there is nothing left to obtain from the
    # second route. Thus, the offspring solution is identical to the first
    # solution.
    offspring = cpp_ox((sol1, sol2), pr107, (0, pr107.num_clients - 1))
    assert_equal(offspring, sol1)

    # When nothing is copied from the first route, everything is taken from
    # the second route. Thus, the offspring solution is identical to the
    # second solution.
    offspring = cpp_ox((sol1, sol2), pr107, (0, 0))
    assert_equal(offspring, sol2)


def test_prize_collecting_instance(prize_collecting):
    """
    Tests that OX functions correctly when there are optional clients.
    """
    data = prize_collecting.replace(vehicle_types=[VehicleType()])

    sol1 = Solution(data, [[4, 5, 6, 7, 8]])
    sol2 = Solution(data, [[2, 3, 4, 5]])

    # From the first route we take [4, 5], which are also in the second route.
    # Thus, we only get the values [2, 3] from the second route, for a total of
    # four clients in the offspring solution.
    offspring = cpp_ox((sol1, sol2), data, (0, 2))
    route = offspring.routes()[0]
    assert_equal(offspring.num_clients(), 4)
    assert_equal(route.visits(), [4, 5, 2, 3])

    # From the first route we take [6, 7], which are not in the second route.
    # From the second route we get [2, 3, 4, 5], for a total of six clients.
    offspring = cpp_ox((sol1, sol2), data, (2, 4))
    route = offspring.routes()[0]
    assert_equal(offspring.num_clients(), 6)
    assert_equal(route.visits(), [6, 7, 2, 3, 4, 5])


def test_empty_solution(prize_collecting):
    """
    Tests that OX returns the other parent when one of the solutions is empty.
    This can occur during prize collecting, and in that case there is nothing
    to exchange via crossover.
    """
    data = prize_collecting.replace(vehicle_types=[VehicleType()])

    cost_evaluator = CostEvaluator(20, 6, 0)
    rng = RandomNumberGenerator(seed=42)

    empty = Solution(data, [])
    nonempty = Solution(data, [[1, 2, 3, 4]])

    # If one of the two parents is empty but the other is not, the returned
    # solution is the nonempty parent.
    for parents in [(empty, nonempty), (nonempty, empty)]:
        offspring = ox(parents, data, cost_evaluator, rng)
        assert_equal(offspring, nonempty)


def test_wrap_around(ok_small):
    """
    Tests that OX wraps around properly on a small instance.
    """
    data = ok_small.replace(vehicle_types=[VehicleType()])

    sol1 = Solution(data, [[1, 3, 2, 4]])
    sol2 = Solution(data, [[4, 1, 2, 3]])

    # Since the start index is bigger than the end index, this call to OX will
    # wrap around. In particular, the offspring inherits the segment [4, 1]
    # from sol1 as [1, *, *, 4], and gets [2, 3] from sol2 as [1, 2, 3, 4].
    offspring = cpp_ox((sol1, sol2), data, (3, 1))
    route = offspring.routes()[0]
    assert_equal(offspring.num_clients(), 4)
    assert_equal(route.visits(), [1, 2, 3, 4])

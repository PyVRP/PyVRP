from numpy.testing import assert_equal, assert_raises

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution, VehicleType
from pyvrp.crossover import ordered_crossover as ox
from pyvrp.crossover._crossover import ordered_crossover as cpp_ox


def test_raises_when_not_tsp(ok_small):
    """
    Tests that the ordered crossover (OX) operator raises when used on a data
    instance that is not a TSP.
    """
    cost_eval = CostEvaluator(20, 6)
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
    # second route. Thus, the offspring solution is indentical to the first
    # solution.
    offspring = cpp_ox((sol1, sol2), pr107, (0, pr107.num_clients - 1))
    assert_equal(offspring, sol1)

    # When nothing is copied from the first route, everything is taken from
    # the second route. Thus, the offspring solution is indentical to the
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
    assert_equal(offspring.num_clients(), 4)

    # From the first route we take [6, 7], which are not in the second route.
    # From the second route we get [2, 3, 4, 5], for a total of six clients.
    offspring = cpp_ox((sol1, sol2), data, (2, 4))
    assert_equal(offspring.num_clients(), 6)

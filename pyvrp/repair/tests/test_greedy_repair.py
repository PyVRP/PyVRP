import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import CostEvaluator, RandomNumberGenerator, Route, Solution
from pyvrp.repair import greedy_repair
from pyvrp.tests.helpers import read


def test_raises_given_no_routes_and_unplanned_clients():
    """
    Tests that the greedy repair function raises when it's not given any routes
    to insert unplanned clients into. Greedy repair does not create new routes,
    so this is an impossible situation.
    """
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)
    empty = Solution(data, [])

    # This call should not raise since unplanned is empty: there are no routes
    # to insert into, which is OK since we have nothing to insert.
    greedy_repair(empty, [], data, cost_eval)

    with assert_raises(ValueError):
        # But now we do need to insert a client, and that should raise.
        greedy_repair(empty, [1], data, cost_eval)


def test_insert_into_empty_route():
    """
    Although greedy repair does not create *new* routes, existing empty routes
    will be used if they're available.
    """
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    # We want to insert client one into an empty route. That should result in
    # a solution that has a single route with just client 1.
    routes = [Route(data, [], 0)]
    greedy = greedy_repair(routes, [1], data, cost_eval)
    assert_equal(greedy, Solution(data, [[1]]))


def test_empty_routes_or_unplanned_is_a_no_op():
    """
    If there are no routes, or no unplanned clients, then the returned solution
    should be the same as the one that was given as an argument.
    """
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    # When unplanned is empty, there is nothing for greedy repair to do, so it
    # should return the exact same solution it received.
    sol = Solution(data, [[3, 2], [1, 4]])
    assert_equal(greedy_repair(sol, [], data, cost_eval), sol)

    # This is also true when the solution is not complete: greedy repair only
    # reinserts what's in unplanned.
    sol = Solution(data, [[2, 3, 4]])
    assert_(not sol.is_complete())
    assert_equal(greedy_repair(sol, [], data, cost_eval), sol)

    # Finally, when both the set of routes and the list of unplanned clients
    # is empty, we get an empty solution.
    assert_equal(greedy_repair([], [], data, cost_eval), Solution(data, []))


def test_after_depot():
    """
    Tests moves where it is optimal to insert directly after the depot.
    """
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    # We want to insert client 4 into the following single-route solution. It
    # is optimal to do so directly after the depot, just before client 3.
    sol = Solution(data, [[3, 2, 1]])
    unplanned = [4]

    # The greedy repair operator inserts into *existing* routes; it does not
    # create new ones.
    repaired = greedy_repair(sol, unplanned, data, cost_eval)
    assert_equal(sol.num_routes(), repaired.num_routes())

    # Let's check if the repaired solution indeed visits client 4 first.
    route = repaired.get_routes()[0]
    assert_equal(route.visits(), [4, 3, 2, 1])


def test_OkSmall():
    """
    Tests greedy repair on a small instance.
    """
    data = read("data/OkSmall.txt")
    cost_eval = CostEvaluator(1, 1)

    # We want to insert 1 and 4 into this solution. Both 1 and 4 are close to
    # 3, so it would be cheapest to insert these into the second route, as
    # 1 -> 3 -> 4.
    sol = Solution(data, [[2], [3]])
    unplanned = [1, 4]

    repaired = greedy_repair(sol, unplanned, data, cost_eval)
    expected = Solution(data, [[2], [4, 3, 1]])
    assert_equal(repaired, expected)


@pytest.mark.parametrize("seed", [0, 13, 42])
def test_RC208(seed: int):
    """
    This smoke test checks that greedy repair is better than random on a larger
    instance, for several seeds.
    """
    data = read("data/RC208.txt", "solomon", "dimacs")
    assert_(data.num_vehicles < data.num_clients)

    # Let's first create a random solution that uses all vehicles.
    rng = RandomNumberGenerator(seed=seed)
    random = Solution.make_random(data, rng)
    assert_equal(random.num_routes(), data.num_vehicles)

    # Let's next create the solution we want to repair. To ensure we use the
    # same number of vehicles, we initialise this solution with dummy routes.
    to_repair = Solution(data, [[idx + 1] for idx in range(data.num_vehicles)])

    cost_eval = CostEvaluator(1, 1)
    unplanned = list(range(data.num_vehicles + 1, data.dimension))

    # Greedily repair the solution by inserting all clients that are not
    # already in the dummy routes.
    greedy = greedy_repair(to_repair, unplanned, data, cost_eval)

    # The greedy solution should be (quite a bit) better than random.
    random_cost = cost_eval.penalised_cost(random)
    greedy_cost = cost_eval.penalised_cost(greedy)
    assert_(greedy_cost < random_cost)

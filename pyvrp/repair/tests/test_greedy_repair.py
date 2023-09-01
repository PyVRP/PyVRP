import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import CostEvaluator, RandomNumberGenerator, Route, Solution
from pyvrp.repair import greedy_repair


def test_raises_given_no_routes_and_unplanned_clients(ok_small):
    """
    Tests that the greedy repair function raises when it's not given any routes
    to insert unplanned clients into. Greedy repair does not create new routes,
    so this is an impossible situation.
    """
    cost_eval = CostEvaluator(1, 1)
    empty = Solution(ok_small, [])

    # This call should not raise since unplanned is empty: there are no routes
    # to insert into, which is OK since we have nothing to insert.
    greedy_repair(empty, [], ok_small, cost_eval)

    with assert_raises(ValueError):
        # But now we do need to insert a client, and that should raise.
        greedy_repair(empty, [1], ok_small, cost_eval)


def test_insert_into_empty_route(ok_small):
    """
    Although greedy repair does not create *new* routes, existing empty routes
    will be used if they're available.
    """
    cost_eval = CostEvaluator(1, 1)

    # We want to insert client one into an empty route. That should result in
    # a solution that has a single route with just client 1.
    routes = [Route(ok_small, [], 0)]
    greedy = greedy_repair(routes, [1], ok_small, cost_eval)
    assert_equal(greedy, Solution(ok_small, [[1]]))


def test_empty_routes_or_unplanned_is_a_no_op(ok_small):
    """
    If there are no routes, or no unplanned clients, then the returned solution
    should be the same as the one that was given as an argument.
    """
    cost_eval = CostEvaluator(1, 1)

    # When unplanned is empty, there is nothing for greedy repair to do, so it
    # should return the exact same solution it received.
    sol = Solution(ok_small, [[3, 2], [1, 4]])
    assert_equal(greedy_repair(sol, [], ok_small, cost_eval), sol)

    # This is also true when the solution is not complete: greedy repair only
    # reinserts what's in unplanned.
    sol = Solution(ok_small, [[2, 3, 4]])
    assert_(not sol.is_complete())
    assert_equal(greedy_repair(sol, [], ok_small, cost_eval), sol)

    # Finally, when both the set of routes and the list of unplanned clients
    # is empty, we get an empty solution.
    assert_equal(greedy_repair([], [], ok_small, cost_eval).num_clients(), 0)


def test_after_depot(ok_small):
    """
    Tests moves where it is optimal to insert directly after the depot.
    """
    cost_eval = CostEvaluator(1, 1)

    # We want to insert client 4 into the following single-route solution. It
    # is optimal to do so directly after the depot, just before client 3.
    sol = Solution(ok_small, [[3, 2, 1]])
    unplanned = [4]

    # The greedy repair operator inserts into *existing* routes; it does not
    # create new ones.
    repaired = greedy_repair(sol, unplanned, ok_small, cost_eval)
    assert_equal(sol.num_routes(), repaired.num_routes())

    # Let's check if the repaired solution indeed visits client 4 first.
    route = repaired.get_routes()[0]
    assert_equal(route.visits(), [4, 3, 2, 1])


def test_OkSmall(ok_small):
    """
    Tests greedy repair on a small instance.
    """
    cost_eval = CostEvaluator(1, 1)

    # We want to insert 1 and 4 into this solution. Both 1 and 4 are close to
    # 3, so it would be cheapest to insert these into the second route, as
    # 1 -> 3 -> 4.
    sol = Solution(ok_small, [[2], [3]])
    unplanned = [1, 4]

    repaired = greedy_repair(sol, unplanned, ok_small, cost_eval)
    expected = Solution(ok_small, [[2], [4, 3, 1]])
    assert_equal(repaired, expected)


@pytest.mark.parametrize("seed", [0, 13, 42])
def test_RC208(rc208, seed: int):
    """
    This smoke test checks that greedy repair is better than random on a larger
    instance, for several seeds.
    """
    assert_(rc208.num_vehicles < rc208.num_clients)

    # Let's first create a random solution that uses all vehicles.
    rng = RandomNumberGenerator(seed=seed)
    random = Solution.make_random(rc208, rng)
    assert_equal(random.num_routes(), rc208.num_vehicles)

    # Let's next create the solution we want to repair. To ensure we use the
    # same number of vehicles, we initialise this solution with dummy routes.
    routes = [[idx + 1] for idx in range(rc208.num_vehicles)]
    to_repair = Solution(rc208, routes)

    cost_eval = CostEvaluator(1, 1)
    unplanned = list(range(rc208.num_vehicles + 1, rc208.num_clients + 1))

    # Greedily repair the solution by inserting all clients that are not
    # already in the dummy routes.
    greedy = greedy_repair(to_repair, unplanned, rc208, cost_eval)

    # The greedy solution should be (quite a bit) better than random.
    random_cost = cost_eval.penalised_cost(random)
    greedy_cost = cost_eval.penalised_cost(greedy)
    assert_(greedy_cost < random_cost)

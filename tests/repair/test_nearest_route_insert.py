import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import CostEvaluator, RandomNumberGenerator, Route, Solution
from pyvrp.repair import nearest_route_insert


def test_raises_given_no_routes_and_unplanned_clients(ok_small):
    """
    Tests that the operator raises when it's not given any routes to insert
    unplanned clients into. The operator does not create new routes, so this is
    an impossible situation.
    """
    cost_eval = CostEvaluator(1, 1, 0)

    # This call should not raise since unplanned is empty: there are no routes
    # to insert into, which is OK since we have nothing to insert.
    nearest_route_insert([], [], ok_small, cost_eval)

    with assert_raises(ValueError):
        # But now we do need to insert a client, and that should raise.
        nearest_route_insert([], [1], ok_small, cost_eval)


def test_insert_into_empty_route(ok_small):
    """
    Although nearest route insert does not create *new* routes, existing empty
    routes will be used if they're the only ones available.
    """
    cost_eval = CostEvaluator(1, 1, 0)

    # We want to insert client one into an empty route. That should result in
    # a single route with just client 1.
    routes = [Route(ok_small, [], 0)]
    repaired = nearest_route_insert(routes, [1], ok_small, cost_eval)
    assert_equal(repaired[0].visits(), [1])


def test_empty_routes_or_unplanned_is_a_no_op(ok_small):
    """
    If there are no routes, or no unplanned clients, then the returned routes
    should be the same as those given as an argument.
    """
    cost_eval = CostEvaluator(1, 1, 0)

    # When unplanned is empty, there is nothing for the operator to do, so it
    # should return the exact same routes it received.
    sol = Solution(ok_small, [[3, 2], [1, 4]])
    assert_equal(
        nearest_route_insert(sol.routes(), [], ok_small, cost_eval),
        sol.routes(),
    )

    # This is also true when the solution is not complete: the operator only
    # reinserts what's in unplanned.
    sol = Solution(ok_small, [[2, 3, 4]])
    assert_(not sol.is_complete())
    assert_equal(
        nearest_route_insert(sol.routes(), [], ok_small, cost_eval),
        sol.routes(),
    )

    # Finally, when both the set of routes and the list of unplanned clients
    # is empty, we get an empty list of routes back.
    assert_equal(nearest_route_insert([], [], ok_small, cost_eval), [])


def test_OkSmall(ok_small):
    """
    Tests nearest route insert on a small instance.
    """
    cost_eval = CostEvaluator(1, 1, 0)

    routes = Solution(ok_small, [[2], [3]]).routes()
    unplanned = [1, 4]

    # We first insert client 1. This client is nearest to client 3, so it
    # gets inserted into 3's route. Then we insert 4. This client is nearer to
    # 2 than it is to centroid([1, 3]), so we insert it into 2's route.
    repaired = nearest_route_insert(routes, unplanned, ok_small, cost_eval)
    repaired_visits = [r.visits() for r in repaired]
    assert_equal(repaired_visits, [[2, 4], [3, 1]])


@pytest.mark.parametrize("seed", [0, 13, 42])
def test_RC208(rc208, seed: int):
    """
    This smoke test checks that nearest route insert is better than random on a
    larger instance, for several seeds.
    """
    assert_(rc208.num_vehicles < rc208.num_clients)

    # Let's first create a random solution that uses all vehicles.
    rng = RandomNumberGenerator(seed=seed)
    random = Solution.make_random(rc208, rng)
    assert_equal(random.num_routes(), rc208.num_vehicles)

    # Let's next create the routes we want to repair. To ensure we use the
    # same number of vehicles, we initialise dummy routes.
    routes = [Route(rc208, [idx + 1], 0) for idx in range(rc208.num_vehicles)]
    unplanned = list(range(rc208.num_vehicles + 1, rc208.num_clients + 1))
    cost_eval = CostEvaluator(1, 1, 0)

    # Repair inserting all clients that are not already in the dummy routes.
    nearest = nearest_route_insert(routes, unplanned, rc208, cost_eval)

    # The repaired routes should be (quite a bit) better than random.
    random_cost = cost_eval.penalised_cost(random)
    nearest_cost = cost_eval.penalised_cost(Solution(rc208, nearest))
    assert_(nearest_cost < random_cost)

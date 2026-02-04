import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator, Depot, ProblemData, VehicleType
from pyvrp.search import ReplaceOptional
from pyvrp.search._search import Node
from tests.helpers import make_search_route


def test_replacing_optional_client():
    """
    Tests that the local search evaluates moves where an optional client is
    replaced with another that is not currently in the solution.
    """
    data = ProblemData(
        clients=[
            Client(0, 0, tw_early=0, tw_late=1, prize=1, required=False),
            Client(0, 0, tw_early=0, tw_late=1, prize=5, required=False),
        ],
        depots=[Depot(0, 0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    route = make_search_route(data, [1])
    assert_equal(route.prizes(), 1)

    op = ReplaceOptional(data)
    cost_eval = CostEvaluator([], 0, 0)

    # Replacing client 1 with 2 yields a prize of 5, rather than 1, at no
    # sadditional cost.
    client2 = Node(loc=2)
    delta, should_apply = op.evaluate(client2, route[1], cost_eval)
    assert_equal(delta, -4)  # +5 prize, -1 prize.
    assert_(should_apply)

    op.apply(client2, route[1])
    route.update()

    assert_equal(route.prizes(), 5)
    assert_(route.is_feasible())


def test_skips_replacing_required_client():
    """
    Tests that ReplaceOptional does not replace a required client.
    """
    data = ProblemData(
        clients=[
            Client(0, 0, tw_early=0, tw_late=1, prize=1, required=True),
            Client(0, 0, tw_early=0, tw_late=1, prize=5, required=False),
        ],
        depots=[Depot(0, 0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    route = make_search_route(data, [1])

    op = ReplaceOptional(data)
    cost_eval = CostEvaluator([], 0, 0)

    # Same example as in previous test but now client 1 is a required client,
    # and cannot be replaced.
    client2 = Node(loc=2)
    assert_equal(op.evaluate(client2, route[1], cost_eval), (0, False))


def test_supports(
    ok_small,
    ok_small_prizes,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that ReplaceOptional supports instances with optional clients.
    """
    assert_(ReplaceOptional.supports(ok_small_prizes))
    assert_(not ReplaceOptional.supports(ok_small))
    assert_(not ReplaceOptional.supports(ok_small_mutually_exclusive_groups))

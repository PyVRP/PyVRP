import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp import (
    Client,
    CostEvaluator,
    Depot,
    Location,
    ProblemData,
    VehicleType,
)
from pyvrp.search import ReplaceOptional
from pyvrp.search._search import Node
from tests.helpers import make_search_route


def test_replacing_optional_client():
    """
    Tests that ReplaceOptional evaluates moves where an optional client is
    replaced with another that is not currently in the solution.
    """
    data = ProblemData(
        locations=[Location(0, 0)],
        clients=[
            Client(0, tw_early=0, tw_late=1, prize=1, required=False),
            Client(0, tw_early=0, tw_late=1, prize=5, required=False),
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((1, 1), dtype=int)],
        duration_matrices=[np.zeros((1, 1), dtype=int)],
    )

    route = make_search_route(data, ["C0"])
    assert_equal(str(route), "C0")

    op = ReplaceOptional(data)
    cost_eval = CostEvaluator([], 0, 0)

    # Replacing C0 with C1 yields a prize of 5, rather than 1, at no additional
    # cost.
    client2 = Node("C1")
    delta, should_apply = op.evaluate(client2, route[1], cost_eval)
    assert_equal(delta, -4)  # +5 prize, -1 prize.
    assert_(should_apply)

    # Apply the move - we should now have only C1 in the route, and no longer
    # C0.
    op.apply(client2, route[1])
    assert_equal(str(route), "C1")


def test_skips_replacing_required_client():
    """
    Tests that ReplaceOptional does not replace a required client.
    """
    data = ProblemData(
        locations=[Location(0, 0)],
        clients=[
            Client(0, tw_early=0, tw_late=1, prize=1, required=True),
            Client(0, tw_early=0, tw_late=1, prize=5, required=False),
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((1, 1), dtype=int)],
        duration_matrices=[np.zeros((1, 1), dtype=int)],
    )

    route = make_search_route(data, ["C0"])

    op = ReplaceOptional(data)
    cost_eval = CostEvaluator([], 0, 0)

    # Same example as in previous test but now C0 is a required client,
    # and cannot be replaced.
    client2 = Node("C1")
    assert_equal(op.evaluate(client2, route[1], cost_eval), (0, False))


def test_skips_assigned_depot_or_missing_other(ok_small_prizes):
    """
    Tests that ReplaceOptional skips assigned clients, depots, or when the
    other client to replace is missing.
    """
    route = make_search_route(ok_small_prizes, ["C0", "C1"])
    assert_(route[1].route)
    assert_(route[2].route)

    # C0 is already assigned, cannot be inserted again.
    op = ReplaceOptional(ok_small_prizes)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], route[2], cost_eval), (0, False))

    # These are not assigned anywhere, so cannot replace.
    node3 = Node("C2")
    node4 = Node("C3")
    assert_equal(op.evaluate(node3, node4, cost_eval), (0, False))

    # This is a depot, which cannot be replaced.
    assert_equal(op.evaluate(route[1], route[0], cost_eval), (0, False))


def test_supports(
    ok_small,
    ok_small_prizes,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that ReplaceOptional supports instances with optional clients, but
    not when those clients are in groups.
    """
    assert_(not ReplaceOptional.supports(ok_small))
    assert_(not ReplaceOptional.supports(ok_small_mutually_exclusive_groups))
    assert_(ReplaceOptional.supports(ok_small_prizes))

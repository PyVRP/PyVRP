import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp import (
    Client,
    ClientGroup,
    CostEvaluator,
    Depot,
    ProblemData,
    VehicleType,
)
from pyvrp.search import InsertOptional
from pyvrp.search._search import Node, Solution
from tests.helpers import make_search_route


def test_inserts_when_makes_sense(prize_collecting):
    """
    Tests that InsertOptional inserts optional clients when that is an
    improving move.
    """
    route = make_search_route(prize_collecting, [1, 2, 3])
    assert_equal(str(route), "1 2 3")

    node = Node(loc=6)
    op = InsertOptional(prize_collecting)
    cost_eval = CostEvaluator([0], 0, 0)

    # dist - prize = dist(3, 6) + dist(6, 0) - dist(3, 0) - prize
    #              = 353 + 114 - 325 - 150
    #              = -8.
    delta, should_apply = op.evaluate(node, route[3], cost_eval)
    assert_equal(delta, -8)
    assert_(should_apply)

    op.apply(node, route[3])
    assert_equal(str(route), "1 2 3 6")


def test_skips_assigned_or_missing(ok_small_prizes):
    """
    Tests that InsertOptional skips assigned clients, or when the other client
    is missing.
    """
    route = make_search_route(ok_small_prizes, [1, 2])
    assert_(route[1].route)
    assert_(route[2].route)

    # Client 1 is already assigned, cannot be inserted again.
    op = InsertOptional(ok_small_prizes)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(route[1], route[2], cost_eval), (0, False))

    # These are not assigned anywhere, so cannot insert after.
    node3 = Node(loc=3)
    node4 = Node(loc=4)
    assert_equal(op.evaluate(node3, node4, cost_eval), (0, False))


def test_supports(
    ok_small_prizes,
    ok_small,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that InsertOptional supports instances with optional clients.
    """
    assert_(not InsertOptional.supports(ok_small))
    assert_(InsertOptional.supports(ok_small_prizes))
    assert_(InsertOptional.supports(ok_small_mutually_exclusive_groups))


def test_group_skip_required(ok_small_mutually_exclusive_groups):
    """
    Tests that InsertOptional skips inserting for required groups, since those
    are already handled via the local search.
    """
    data = ok_small_mutually_exclusive_groups
    assert_(data.group(0).required)

    solution = Solution(data)
    route = solution.routes[0]

    op = InsertOptional(data)
    op.init(solution)

    # The group is required, and that means the operator skips it: required
    # groups are handled in the local search, and should always be present.
    node = Node(loc=1)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(node, route[0], cost_eval), (0, False))


def test_group_skip_duplicates():
    """
    Tests that InsertOptional skips inserting for groups when they are already
    present in the solution.
    """
    data = ProblemData(
        clients=[
            Client(0, 0, prize=1, required=False, group=0),
            Client(0, 0, prize=1, required=False, group=0),
        ],
        depots=[Depot(0, 0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
        groups=[ClientGroup([1, 2], required=False)],
    )

    solution = Solution(data)
    route = solution.routes[0]

    op = InsertOptional(data)
    op.init(solution)

    # Group is not yet in the solution. Inserting it via the first client
    # yields a prize value, and is thus an improving move.
    node = solution.nodes[1]
    cost_eval = CostEvaluator([], 0, 0)
    assert_equal(op.evaluate(node, route[0], cost_eval), (-1, True))
    op.apply(node, route[0])
    route.update()

    # Trying this again, now with the second client (in the same group) is not
    # possible, because the group is already in the solution.
    node = solution.nodes[2]
    cost_eval = CostEvaluator([], 0, 0)
    assert_equal(op.evaluate(node, route[0], cost_eval), (0, False))


def test_insert_in_empty_routes_considers_fixed_vehicle_cost():
    """
    Tests that InsertOptional considers fixed vehicle costs when routes are
    empty.
    """
    cost_eval = CostEvaluator([], 0, 0)
    data = ProblemData(
        clients=[
            Client(x=1, y=1, required=False),
            Client(x=1, y=0, required=False),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(fixed_cost=7), VehicleType(fixed_cost=13)],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    op = InsertOptional(data)

    # All distances, durations, and loads are equal. So the only cost change
    # can happen due to vehicle changes. In this, case we evaluate inserting a
    # client into an empty route. That adds the fixed vehicle cost of 7 for
    # this vehicle type.
    route = make_search_route(data, [], vehicle_type=0)
    assert_equal(op.evaluate(Node(loc=1), route[0], cost_eval), (7, False))

    # Same story for this route, but now we have a different vehicle type with
    # fixed cost 13.
    route = make_search_route(data, [], vehicle_type=1)
    assert_equal(op.evaluate(Node(loc=1), route[0], cost_eval), (13, False))

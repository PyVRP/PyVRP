import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp import Client, CostEvaluator, Depot, ProblemData, VehicleType
from pyvrp.search import RemoveOptional
from pyvrp.search._search import Solution
from tests.helpers import make_search_route


def test_does_not_remove_required_clients():
    """
    Tests that RemoveOptional does not remove required clients, even when that
    might result in a significant cost improvement.
    """
    data = ProblemData(
        clients=[
            # This client cannot be removed, even though it causes significant
            # load violations.
            Client(x=1, y=1, delivery=[100], required=True),
            # This client can and should be removed, because the prize is not
            # worth the detour.
            Client(x=2, y=2, delivery=[0], prize=0, required=False),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(1, capacity=[50])],
        distance_matrices=[np.where(np.eye(3), 0, 10)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )

    route = make_search_route(data, [1, 2])
    cost_eval = CostEvaluator([100], 100, 0)
    op = RemoveOptional(data)

    # Test that the the operator cannot remove the first required client, but
    # does want to remove the second.
    assert_equal(op.evaluate(route[1], cost_eval), (0, False))
    assert_equal(op.evaluate(route[2], cost_eval), (-10, True))

    # Should remove the second client.
    op.apply(route[2])
    assert_equal(str(route), "1")


def test_supports(
    ok_small,
    ok_small_prizes,
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that RemoveOptional supports prize-collecting instances, but not
    instances with only required clients.
    """
    assert_(not RemoveOptional.supports(ok_small))
    assert_(RemoveOptional.supports(ok_small_prizes))
    assert_(RemoveOptional.supports(ok_small_mutually_exclusive_groups))


def test_skip_only_in_required_group(ok_small_mutually_exclusive_groups):
    """
    Tests that RemoveOptional only removes clients in required groups when
    there is more than one group client in the solution.
    """
    data = ok_small_mutually_exclusive_groups
    solution = Solution(data)
    route = solution.routes[0]
    route.append(solution.nodes[1])
    route.append(solution.nodes[2])
    route.update()

    op = RemoveOptional(data)
    cost_eval = CostEvaluator([0], 0, 0)
    op.init(solution)

    # The mutually exclusive group is in the solution twice, with both clients
    # 1 and 2. That's infeasible, and removing either is an improving move.
    assert_equal(op.evaluate(route[1], cost_eval), (-1_592, True))
    assert_equal(op.evaluate(route[2], cost_eval), (-2_231, True))

    # Remove the second client. There's now only one client left in the
    # solution for this group.
    op.apply(route[2])
    route.update()

    # And that means we cannot remove the last client, since that would render
    # the solution group infeasible.
    assert_equal(op.evaluate(route[1], cost_eval), (0, False))

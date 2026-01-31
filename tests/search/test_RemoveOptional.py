import numpy as np
from numpy.testing import assert_equal

from pyvrp import (
    Client,
    CostEvaluator,
    Depot,
    ProblemData,
    VehicleType,
)
from pyvrp.search import RemoveOptional
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
    assert_equal(op.evaluate(route[1], cost_eval), 0)
    assert_equal(op.evaluate(route[2], cost_eval), -10)

    # Should remove the second client.
    op.apply(route[2])
    assert_equal(str(route), "1")

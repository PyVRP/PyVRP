import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator
from pyvrp.search import RelocatePickup
from pyvrp.search._search import Route, Solution

_INT_MAX = np.iinfo(np.int64).max


def test_relocate(small_shipments):
    """
    TODO
    """
    pass


def test_reload_depot(small_shipments):
    """
    TODO
    """
    pass


def test_supports(ok_small, small_shipments, small_optional_shipments):
    """
    Tests that the operator supports instances with shipments.
    """
    assert_(RelocatePickup.supports(small_shipments))
    assert_(RelocatePickup.supports(small_optional_shipments))

    # This instance has no shipments.
    assert_(not RelocatePickup.supports(ok_small))


def test_name(small_shipments):
    """
    Tests the operator's name property.
    """
    op = RelocatePickup(small_shipments)
    assert_equal(op.name, "RelocatePickup")


def test_cannot_improve_singleton_route(small_shipments):
    """
    Tests that the operator cannot improve a singleton route.
    """
    sol = Solution(small_shipments)
    pickup, delivery = sol.shipments[0]

    route = Route(small_shipments, 0)
    route.append(pickup)
    route.append(delivery)
    route.update()

    op = RelocatePickup(small_shipments)
    cost_eval = CostEvaluator([0], 0, 0)
    assert_equal(op.evaluate(pickup, cost_eval), (_INT_MAX, False))

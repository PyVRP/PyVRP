from numpy.testing import assert_, assert_equal

from pyvrp import VehicleType
from pyvrp.search import SwapShipment


def test_swap_direct_pairs():
    """
    TODO
    """
    pass


def test_swap_where_u_is_a_direct_pair():
    """
    TODO
    """
    pass


def test_swap_where_v_is_a_direct_pair():
    """
    TODO
    """
    pass


def test_swap_general():
    """
    TODO
    """
    pass


def test_v_is_delivery():
    """
    TODO
    """
    pass


def test_skips_clients_and_non_shipments():
    """
    TODO
    """
    pass


def test_supports(ok_small, gtsp, small_shipments):
    """
    Tests that the operator supports instances with multiple routes and
    shipments.
    """
    # These instances have no shipments.
    assert_(not SwapShipment.supports(ok_small))
    assert_(not SwapShipment.supports(gtsp))

    # This instance has shipments and more than one vehicle, so the operator
    # supports it.
    assert_(SwapShipment.supports(small_shipments))

    # Now let's modify the instance to have just one vehicle. The operator does
    # not support TSP, so it does not support the modified instance.
    data = small_shipments.replace(vehicle_types=[VehicleType(capacity=[0])])
    assert_equal(data.num_vehicles, 1)
    assert_(not SwapShipment.supports(data))


def test_name(small_shipments):
    """
    Tests the operator's name.
    """
    op = SwapShipment(small_shipments)
    assert_equal(op.name, "SwapShipment")

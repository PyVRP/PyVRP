from numpy.testing import assert_

from pyvrp.search import RemoveOptionalShipment


def test_supports(ok_small, small_shipments, small_optional_shipments):
    """
    Tests supports().
    """
    # This instance only has clients, not shipments.
    assert_(not RemoveOptionalShipment.supports(ok_small))

    # The operator only supports instances with *optional* shipments.
    assert_(not RemoveOptionalShipment.supports(small_shipments))
    assert_(RemoveOptionalShipment.supports(small_optional_shipments))

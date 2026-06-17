from numpy.testing import assert_

from pyvrp.search import ReplaceOptionalShipment


def test_replace_adjacent():
    """
    TODO
    """
    pass


def test_replace():
    """
    TODO
    """
    pass


def test_supports(ok_small, small_shipments, small_optional_shipments):
    """
    Tests supports().
    """
    # This instance only has clients, not shipments.
    assert_(not ReplaceOptionalShipment.supports(ok_small))

    # The operator only supports instances with *optional* shipments.
    assert_(not ReplaceOptionalShipment.supports(small_shipments))
    assert_(ReplaceOptionalShipment.supports(small_optional_shipments))

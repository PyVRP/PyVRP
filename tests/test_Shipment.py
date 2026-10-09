import pickle

import numpy as np
import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import Shipment, ShipmentStep

_INT_MAX = np.iinfo(np.int64).max


@pytest.mark.parametrize(
    ("tw_early", "tw_late", "service_duration"),
    [
        (1, 0, 0),  # tw_early < tw_late
        (-1, 0, 0),  # tw_early < 0
        (0, 1, -1),  # service_duration < 0
    ],
)
def test_raises_invalid_shipment_step(
    tw_early: int,
    tw_late: int,
    service_duration: int,
):
    """
    Tests that the ShipmentStep constructor raises for invalid arguments.
    """
    with assert_raises(ValueError):
        ShipmentStep(0, tw_early, tw_late, service_duration)


@pytest.mark.parametrize(
    (
        "pickup_tw_early",
        "pickup_tw_late",
        "pickup_service_duration",
        "delivery_tw_early",
        "delivery_tw_late",
        "delivery_service_duration",
        "amount",
        "prize",
    ),
    [
        (1, 0, 0, 0, 0, 0, [], 0),  # pickup tw_early < tw_late
        (-1, 0, 0, 0, 0, 0, [], 0),  # pickup tw_early < 0
        (0, 0, -1, 0, 0, 0, [], 0),  # pickup service_duration < 0
        (0, 0, 0, 1, 0, 0, [], 0),  # delivery tw_early < tw_late
        (0, 0, 0, -1, 0, 0, [], 0),  # delivery tw_early < 0
        (0, 0, 0, 0, 0, -1, [], 0),  # delivery service_duration < 0
        (10, 15, 0, 0, 5, 0, [], 0),  # delivery tw_late < pickup tw_early
        (0, 0, 0, 0, 0, 0, [-1], 0),  # amount < 0
        (0, 0, 0, 0, 0, 0, [], -1),  # prize < 0
    ],
)
def test_raises_invalid_shipment(
    pickup_tw_early: int,
    pickup_tw_late: int,
    pickup_service_duration: int,
    delivery_tw_early: int,
    delivery_tw_late: int,
    delivery_service_duration: int,
    amount: list[int],
    prize: int,
):
    """
    Tests that the ShipmentStep constructor raises for invalid arguments.
    """
    with assert_raises(ValueError):
        Shipment(
            0,
            0,
            pickup_tw_early,
            pickup_tw_late,
            pickup_service_duration,
            delivery_tw_early,
            delivery_tw_late,
            delivery_service_duration,
            amount,
            prize,
        )


def test_default_attributes():
    """
    Tests the default attributes on shipments.
    """
    shipment = Shipment(0, 0)
    assert_equal(shipment.pickup.location, 0)
    assert_equal(shipment.pickup.tw_early, 0)
    assert_equal(shipment.pickup.tw_late, _INT_MAX)
    assert_equal(shipment.pickup.service_duration, 0)
    assert_equal(shipment.delivery.location, 0)
    assert_equal(shipment.delivery.tw_early, 0)
    assert_equal(shipment.delivery.tw_late, _INT_MAX)
    assert_equal(shipment.delivery.service_duration, 0)
    assert_equal(shipment.amount, [])
    assert_equal(shipment.prize, 0)
    assert_(shipment.required)
    assert_equal(shipment.name, "")


def test_custom_attributes():
    """
    Tests all non-default attributes are correctly set on the constructed
    instance.
    """
    shipment = Shipment(
        pickup_location=1,
        delivery_location=2,
        pickup_tw_early=3,
        pickup_tw_late=5,
        pickup_service_duration=7,
        delivery_tw_early=11,
        delivery_tw_late=13,
        delivery_service_duration=17,
        amount=[19],
        prize=23,
        required=False,
        name="test",
    )

    assert_equal(shipment.pickup.location, 1)
    assert_equal(shipment.delivery.location, 2)
    assert_equal(shipment.pickup.tw_early, 3)
    assert_equal(shipment.pickup.tw_late, 5)
    assert_equal(shipment.pickup.service_duration, 7)
    assert_equal(shipment.delivery.tw_early, 11)
    assert_equal(shipment.delivery.tw_late, 13)
    assert_equal(shipment.delivery.service_duration, 17)
    assert_equal(shipment.amount, [19])
    assert_equal(shipment.prize, 23)
    assert_(not shipment.required)
    assert_equal(shipment.name, "test")
    assert_equal(str(shipment), "test")


def test_shipment_step_eq():
    """
    Tests ShipmentStep's equality operator.
    """
    step1 = ShipmentStep(0, 1, 2, 3)
    assert_(step1 == step1)  # should equal itself

    step2 = ShipmentStep(4, 5, 6, 7)
    assert_(step2 != step1)  # step2 is different

    step3 = ShipmentStep(0, 1, 2, 3)
    assert_(step1 == step3)  # step3 is the same as step1

    assert_(step1 != object())
    assert_(step1 != "123")


def test_shipment_eq():
    """
    Tests Shipment's equality operator.
    """
    shipment1 = Shipment(0, 1, prize=5, required=False, name="test")
    assert_(shipment1 == shipment1)  # should equal itself

    shipment2 = Shipment(0, 1, prize=5, required=False, name="other")
    assert_(shipment1 != shipment2)  # shipment2 is different

    shipment3 = Shipment(0, 1, prize=5, required=False, name="test")
    assert_(shipment1 == shipment3)  # shipment3 is the same as shipment1

    assert_(shipment1 != object())
    assert_(shipment1 != "123")


def test_pickle():
    """
    Tests pickling and unpickling shipments.
    """
    shipment = Shipment(0, 1, name="test")

    pickled = pickle.dumps(shipment)
    unpickled = pickle.loads(pickled)
    assert_equal(unpickled, shipment)

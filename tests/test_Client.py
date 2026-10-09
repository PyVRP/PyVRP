import pickle

import pytest
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import Client


@pytest.mark.parametrize(
    (
        "location",
        "delivery",
        "pickup",
        "service_duration",
        "tw_early",
        "tw_late",
        "release_time",
        "prize",
        "required",
        "group",
        "name",
    ),
    [
        (1, 1, 1, 1, 0, 1, 0, 0, True, None, "test name"),  # normal
        (1, 1, 0, 0, 0, 1, 0, 0, True, None, "1234"),  # zero duration
        (1, 0, 0, 1, 0, 1, 0, 0, True, None, "1,2,3,4"),  # zero delivery
        (1, 1, 0, 1, 0, 0, 0, 0, True, None, ""),  # zero time windows
        (1, 1, 0, 1, 0, 1, 1, 0, True, None, ""),  # positive release time
        (0, 1, 0, 1, 0, 1, 0, 1, True, None, ""),  # positive prize
        (0, 1, 0, 1, 0, 1, 0, 1, False, None, ""),  # not required
        (0, 1, 0, 1, 0, 1, 0, 1, False, 0, ""),  # group membership
    ],
)
def test_constructor_initialises_data_fields_correctly(
    location: int,
    delivery: int,
    pickup: int,
    service_duration: int,
    tw_early: int,
    tw_late: int,
    release_time: int,
    prize: int,
    required: bool,
    group: int | None,
    name: str,
):
    """
    Tests that the access properties return the data that was given to the
    Client's constructor.
    """
    client = Client(
        location=location,
        delivery=[delivery],
        pickup=[pickup],
        service_duration=service_duration,
        tw_early=tw_early,
        tw_late=tw_late,
        release_time=release_time,
        prize=prize,
        required=required,
        group=group,
        name=name,
    )

    assert_equal(client.location, location)
    assert_equal(client.delivery, [delivery])
    assert_equal(client.pickup, [pickup])
    assert_equal(client.service_duration, service_duration)
    assert_equal(client.tw_early, tw_early)
    assert_equal(client.tw_late, tw_late)
    assert_equal(client.release_time, release_time)
    assert_equal(client.prize, prize)
    assert_equal(client.required, required)
    assert_equal(client.group, group)
    assert_equal(client.name, name)
    assert_equal(str(client), name)


@pytest.mark.parametrize(
    (
        "delivery",
        "pickup",
        "service",
        "tw_early",
        "tw_late",
        "release_time",
        "prize",
    ),
    [
        (1, 0, 0, 1, 0, 0, 0),  # late < early
        (1, 0, 0, -1, 0, 0, 0),  # negative early
        (0, 0, -1, 0, 1, 0, 1),  # negative service duration
        (-1, 0, 1, 0, 1, 0, 0),  # negative delivery
        (0, -1, 1, 0, 1, 0, 0),  # negative pickup
        (0, 0, 0, 0, 1, -1, 0),  # negative release time
        (0, 0, 0, 0, 1, 2, 0),  # release time > late
        (1, 0, 1, 0, 1, 0, -1),  # negative prize
    ],
)
def test_raises_for_invalid_data(
    delivery: int,
    pickup: int,
    service: int,
    tw_early: int,
    tw_late: int,
    release_time: int,
    prize: int,
):
    """
    Tests that invalid configurations are not accepted.
    """
    with assert_raises(ValueError):
        Client(
            0,
            [delivery],
            [pickup],
            service,
            tw_early,
            tw_late,
            release_time,
            prize,
        )


def test_eq():
    """
    Tests the equality operator.
    """
    client1 = Client(location=0, delivery=[1], pickup=[2], tw_late=3, group=0)
    client2 = Client(location=0, delivery=[1], pickup=[2], tw_late=3, group=1)
    assert_(client1 != client2)

    # This client is equivalent to client1.
    client3 = Client(location=0, delivery=[1], pickup=[2], tw_late=3, group=0)
    assert_(client1 == client3)
    assert_(client3 == client3)

    # And some things that are not clients.
    assert_(client1 != "text")
    assert_(client1 != 1)


def test_eq_name():
    """
    Tests that the equality operator considers names.
    """
    assert_(Client(0, name="1") != Client(0, name="2"))


def test_pickle():
    """
    Tests that clients can be serialised and unserialised.
    """
    before_pickle = Client(location=0, name="test")
    bytes = pickle.dumps(before_pickle)
    assert_equal(pickle.loads(bytes), before_pickle)


@pytest.mark.parametrize(
    ("delivery", "pickup", "exp_delivery", "exp_pickup"),
    [
        ([0], [0], [0], [0]),
        ([0], [0, 1, 2], [0, 0, 0], [0, 1, 2]),
        ([0, 1, 2], [0], [0, 1, 2], [0, 0, 0]),
        ([0, 2], [1], [0, 2], [1, 0]),
        ([], [], [], []),
    ],
)
def test_client_load_dimensions_are_padded_with_zeroes(
    delivery: list[int],
    pickup: list[int],
    exp_delivery: list[int],
    exp_pickup: list[int],
):
    """
    Tests that any missing load dimensions for the pickup and delivery Client
    arguments are padded with zeroes.
    """
    client = Client(0, delivery=delivery, pickup=pickup)
    assert_equal(client.delivery, exp_delivery)
    assert_equal(client.pickup, exp_pickup)

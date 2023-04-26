from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import Client, ProblemData


@mark.parametrize(
    "x,y,demand,service_duration,tw_early,tw_late",
    [
        (1, 1, 1, 1, 0, 1),  # normal
        (1, 1, 1, 0, 0, 1),  # zero duration
        (1, 1, 0, 1, 0, 1),  # zero demand
        (1, 1, 1, 1, 0, 0),  # zero length time interval
        (-1, -1, 1, 1, 0, 1),  # negative coordinates
    ],
)
def test_valid_client(
    x: int,
    y: int,
    demand: int,
    service_duration: int,
    tw_early: int,
    tw_late: int,
):
    client = Client(x, y, demand, service_duration, tw_early, tw_late)
    assert_equal(client.x, x)
    assert_equal(client.y, y)
    assert_equal(client.demand, demand)
    assert_equal(client.service_duration, service_duration)
    assert_equal(client.tw_early, tw_early)
    assert_equal(client.tw_late, tw_late)


@mark.parametrize(
    "x,y,demand,service_duration,tw_early,tw_late",
    [
        (1, 1, 1, 1, 1, 0),  # late < early
        (1, 1, 1, -1, 0, 1),  # negative duration
        (1, 1, -1, 1, 0, 1),  # negative demand
    ],
)
def test_invalid_client(
    x: int,
    y: int,
    demand: int,
    service_duration: int,
    tw_early: int,
    tw_late: int,
):
    with assert_raises(ValueError):
        Client(x, y, demand, service_duration, tw_early, tw_late)


def test_depot_is_first_client():
    """
    The ``depot()`` helper should return the first client, that is,
    ``client(0)``.
    """
    depot = Client(x=0, y=0)
    clients = [
        depot,
        Client(x=0, y=1),
    ]
    data = ProblemData(
        clients=clients,
        nb_vehicles=1,
        vehicle_cap=1,
        duration_matrix=[[0, 1], [1, 0]],
    )

    assert_(data.depot() is data.client(0))
    # Note that the constructor makes a copy of the objects
    assert_(data.depot() is not depot)

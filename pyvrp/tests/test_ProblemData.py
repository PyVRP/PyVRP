from numpy.testing import assert_, assert_allclose, assert_raises
from pytest import mark

from pyvrp import Client, ProblemData


@mark.parametrize(
    "x,y,demand,service_duration,tw_early,tw_late,prize",
    [
        (1, 1, 1, 1, 0, 1, 0),  # normal
        (1, 1, 1, 0, 0, 1, 0),  # zero duration
        (1, 1, 0, 1, 0, 1, 0),  # zero demand
        (1, 1, 1, 1, 0, 0, 0),  # zero length time interval
        (-1, -1, 1, 1, 0, 1, 0),  # negative coordinates
        (0, 0, 1, 1, 0, 1, 1),  # positive prize
    ],
)
def test_client_constructor_initialises_data_fields_correctly(
    x: int,
    y: int,
    demand: int,
    service_duration: int,
    tw_early: int,
    tw_late: int,
    prize: int,
):
    client = Client(x, y, demand, service_duration, tw_early, tw_late, prize)
    assert_allclose(client.x, x)
    assert_allclose(client.y, y)
    assert_allclose(client.demand, demand)
    assert_allclose(client.service_duration, service_duration)
    assert_allclose(client.tw_early, tw_early)
    assert_allclose(client.tw_late, tw_late)
    assert_allclose(client.prize, prize)


@mark.parametrize(
    "x,y,demand,service_duration,tw_early,tw_late,prize",
    [
        (1, 1, 1, 1, 1, 0, 0),  # late < early
        (1, 1, 1, -1, 0, 1, 0),  # negative duration
        (1, 1, -1, 1, 0, 1, 0),  # negative demand
        (1, 1, 1, 1, 0, 1, -1),  # negative prize
    ],
)
def test_raises_for_invalid_client_data(
    x: int,
    y: int,
    demand: int,
    service_duration: int,
    tw_early: int,
    tw_late: int,
    prize: int,
):
    with assert_raises(ValueError):
        Client(x, y, demand, service_duration, tw_early, tw_late, prize)


def test_depot_is_first_client():
    """
    The ``depot()`` helper should return the first client, that is,
    ``client(0)``.
    """
    mat = [[0, 1], [1, 0]]

    data = ProblemData(
        clients=[Client(x=0, y=0), Client(x=0, y=1)],
        num_vehicles=1,
        vehicle_cap=1,
        distance_matrix=mat,
        duration_matrix=mat,
    )

    assert_(data.depot() is data.client(0))

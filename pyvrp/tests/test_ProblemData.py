import numpy as np
from numpy.random import default_rng
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
def test_client_constructor_initialises_data_fields_correctly(
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
def test_raises_for_invalid_client_data(
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
    mat = [[0, 1], [1, 0]]
    depot = Client(x=0, y=0)
    clients = [
        depot,
        Client(x=0, y=1),
    ]

    data = ProblemData(
        clients=clients,
        nb_vehicles=1,
        vehicle_cap=1,
        distance_matrix=mat,
        duration_matrix=mat,
    )

    assert_(data.depot() is data.client(0))


def test_matrix_access():
    """
    Tests that the ``duration()``, ``duration_matrix()``, ``dist()``, and
    ``distance_matrix()`` methods correctly index the underlying duration
    and distance matrices.
    """
    size = 6
    gen = default_rng(seed=42)
    dist_mat = gen.integers(500, size=(size, size))
    dur_mat = gen.integers(500, size=(size, size))

    assert_(not np.allclose(dist_mat, dur_mat))

    clients = [
        Client(x=0, y=0, demand=0, service_duration=0, tw_early=0, tw_late=10)
        for _ in range(size)
    ]

    data = ProblemData(
        clients=clients,
        nb_vehicles=1,
        vehicle_cap=1,
        distance_matrix=dist_mat,  # type: ignore
        duration_matrix=dur_mat,  # type: ignore
    )

    dist_mat_data = data.distance_matrix()
    dur_mat_data = data.duration_matrix()

    for frm in range(size):
        for to in range(size):
            assert_equal(dur_mat[frm, to], data.duration(frm, to))
            assert_equal(dist_mat[frm, to], data.dist(frm, to))

            assert_equal(dur_mat_data[frm, to], dur_mat[frm, to])
            assert_equal(dist_mat_data[frm, to], dist_mat[frm, to])

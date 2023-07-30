import numpy as np
from numpy.random import default_rng
from numpy.testing import assert_allclose, assert_raises
from pytest import mark

from pyvrp import Client, ProblemData, VehicleType
from pyvrp.tests.helpers import read


@mark.parametrize(
    (
        "x",
        "y",
        "demand",
        "service_duration",
        "tw_early",
        "tw_late",
        "release_time",
        "prize",
    ),
    [
        (1, 1, 1, 1, 0, 1, 0, 0),  # normal
        (1, 1, 1, 0, 0, 1, 0, 0),  # zero duration
        (1, 1, 0, 1, 0, 1, 0, 0),  # zero demand
        (1, 1, 1, 1, 0, 0, 0, 0),  # zero length time interval
        (-1, -1, 1, 1, 0, 1, 0, 0),  # negative coordinates
        (1, 1, 1, 1, 0, 1, 1, 0),  # positive release time
        (0, 0, 1, 1, 0, 1, 0, 1),  # positive prize
    ],
)
def test_client_constructor_initialises_data_fields_correctly(
    x: int,
    y: int,
    demand: int,
    service_duration: int,
    tw_early: int,
    tw_late: int,
    release_time: int,
    prize: int,
):
    client = Client(
        x, y, demand, service_duration, tw_early, tw_late, release_time, prize
    )
    assert_allclose(client.x, x)
    assert_allclose(client.y, y)
    assert_allclose(client.demand, demand)
    assert_allclose(client.service_duration, service_duration)
    assert_allclose(client.tw_early, tw_early)
    assert_allclose(client.tw_late, tw_late)
    assert_allclose(client.release_time, release_time)
    assert_allclose(client.prize, prize)


@mark.parametrize(
    (
        "x",
        "y",
        "demand",
        "service",
        "tw_early",
        "tw_late",
        "release_time",
        "prize",
    ),
    [
        (1, 1, 1, 1, 1, 0, 0, 0),  # late < early
        (1, 1, 1, -1, 0, 1, 0, 0),  # negative service duration
        (1, 1, -1, 1, 0, 1, 0, 0),  # negative demand
        (1, 1, 1, 1, 0, 1, 0, -1),  # negative prize
    ],
)
def test_raises_for_invalid_client_data(
    x: int,
    y: int,
    demand: int,
    service: int,
    tw_early: int,
    tw_late: int,
    release_time: int,
    prize: int,
):
    with assert_raises(ValueError):
        Client(x, y, demand, service, tw_early, tw_late, release_time, prize)


@mark.parametrize(
    "x,y,demand,service,tw_early,tw_late,release_time,prize",
    [
        (0, 0, 1, 0, 0, 0, 0, 0),  # demand != 0
        (0, 0, 0, 1, 0, 0, 0, 0),  # service duration != 0
        (0, 0, 0, 0, 0, 0, 1, 0),  # release time != 0
    ],
)
def test_raises_for_invalid_depot_data(
    x: int,
    y: int,
    demand: int,
    service: int,
    tw_early: int,
    tw_late: int,
    release_time: int,
    prize: int,
):
    depot = Client(
        x, y, demand, service, tw_early, tw_late, release_time, prize
    )

    with assert_raises(ValueError):
        ProblemData([depot], [VehicleType(1, 2)], [[0]], [[0]])


def test_problem_data_raises_when_no_clients_provided():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    no clients are provided.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            vehicle_types=[VehicleType(1, 2)],
            distance_matrix=[],
            duration_matrix=[],
        )

    # One client (the depot) should not raise.
    ProblemData(
        clients=[Client(x=0, y=0)],
        vehicle_types=[VehicleType(1, 2)],
        distance_matrix=[[0]],
        duration_matrix=[[0]],
    )


@mark.parametrize(
    "matrix",
    [
        [[0, 0]],  # num rows < num clients
        [[], []],  # num cols < num clients
        [[0, 0], [0, 0], [0, 0]],  # num rows > num clients
        [[0, 0, 0], [0, 0, 0]],  # num cols > num clients
    ],
)
def test_problem_data_raises_when_incorrect_matrix_dimensions(matrix):
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    the distance or duration matrix does not match the number of clients in
    dimension size.
    """
    clients = [Client(x=0, y=0), Client(x=0, y=0)]
    vehicle_types = [VehicleType(1, 2)]

    with assert_raises(ValueError):
        ProblemData(clients, vehicle_types, matrix, [[0, 0], [0, 0]])

    with assert_raises(ValueError):
        ProblemData(clients, vehicle_types, [[0, 0], [0, 0]], matrix)


def test_centroid():
    data = read("data/OkSmall.txt")

    centroid = data.centroid()
    x = [data.client(idx).x for idx in range(1, data.num_clients + 1)]
    y = [data.client(idx).y for idx in range(1, data.num_clients + 1)]

    assert_allclose(centroid[0], np.mean(x))
    assert_allclose(centroid[1], np.mean(y))


def test_matrix_access():
    """
    Tests that the ``duration()`` and ``dist()`` methods correctly index the
    underlying duration and distance matrices.
    """
    gen = default_rng(seed=42)
    size = 6

    dist_mat = gen.integers(500, size=(size, size))
    dur_mat = gen.integers(500, size=(size, size))
    clients = [
        Client(x=0, y=0, demand=0, service_duration=0, tw_early=0, tw_late=10)
        for _ in range(size)
    ]

    data = ProblemData(
        clients=clients,
        vehicle_types=[VehicleType(1, 2)],
        distance_matrix=dist_mat,  # type: ignore
        duration_matrix=dur_mat,  # type: ignore
    )

    for frm in range(size):
        for to in range(size):
            assert_allclose(dur_mat[frm, to], data.duration(frm, to))
            assert_allclose(dist_mat[frm, to], data.dist(frm, to))

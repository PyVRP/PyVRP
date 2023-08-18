import numpy as np
from numpy.random import default_rng
from numpy.testing import assert_, assert_allclose, assert_raises
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
    """
    Tests that the access properties return the data that was given to the
    Client's constructor.
    """
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
    """
    Tests that invalid configurations are not accepted.
    """
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
    """
    Tests that an invalid depot configuration is not accepted.
    """
    depot = Client(
        x, y, demand, service, tw_early, tw_late, release_time, prize
    )

    with assert_raises(ValueError):
        ProblemData(
            clients=[depot],
            vehicle_types=[VehicleType(1, 2)],
            distance_matrix=np.asarray([[0]], dtype=int),
            duration_matrix=np.asarray([[0]], dtype=int),
        )


def test_problem_data_raises_when_no_clients_provided():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    no clients are provided.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            vehicle_types=[VehicleType(1, 2)],
            distance_matrix=np.asarray([[]], dtype=int),
            duration_matrix=np.asarray([[]], dtype=int),
        )

    # One client (the depot) should not raise.
    ProblemData(
        clients=[Client(x=0, y=0)],
        vehicle_types=[VehicleType(1, 2)],
        distance_matrix=np.asarray([[0]]),
        duration_matrix=np.asarray([[0]]),
    )


@mark.parametrize(
    "matrix",
    [
        np.asarray([[0, 0]]),  # num rows < num clients
        np.asarray([[], []]),  # num cols < num clients
        np.asarray([[0, 0], [0, 0], [0, 0]]),  # num rows > num clients
        np.asarray([[0, 0, 0], [0, 0, 0]]),  # num cols > num clients
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
    other_matrix = np.zeros((2, 2), dtype=int)  # this one's OK

    with assert_raises(ValueError):
        ProblemData(clients, vehicle_types, matrix, other_matrix)

    with assert_raises(ValueError):
        ProblemData(clients, vehicle_types, other_matrix, matrix)


def test_centroid():
    """
    Tests the computation of the centroid of all clients in the data instance.
    """
    data = read("data/OkSmall.txt")

    centroid = data.centroid()
    x = [data.client(idx).x for idx in range(1, data.num_clients + 1)]
    y = [data.client(idx).y for idx in range(1, data.num_clients + 1)]

    assert_allclose(centroid[0], np.mean(x))
    assert_allclose(centroid[1], np.mean(y))


def test_matrix_access():
    """
    Tests that the ``duration()`` and ``dist()`` methods (and their matrix
    access equivalents) correctly index the underlying data matrices.
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
        distance_matrix=dist_mat,
        duration_matrix=dur_mat,
    )

    assert_allclose(data.distance_matrix(), dist_mat)
    assert_allclose(data.duration_matrix(), dur_mat)

    for frm in range(size):
        for to in range(size):
            assert_allclose(data.dist(frm, to), dist_mat[frm, to])
            assert_allclose(data.duration(frm, to), dur_mat[frm, to])


def test_matrices_are_not_writeable():
    """
    Tests that the data matrices provided by ``distance_matrix()`` and
    ``duration_matrix()`` are not writeable. They can be read from, but
    assigning new values should raise an error.

    We require this because they're constant on the C++ side, and allowing
    changes from Python causes undefined behaviour on the C++ side.
    """
    data = ProblemData(
        clients=[Client(x=0, y=0)],
        vehicle_types=[VehicleType(1, 2)],
        distance_matrix=np.array([[0]]),
        duration_matrix=np.array([[0]]),
    )

    dist_mat = data.distance_matrix()
    dur_mat = data.duration_matrix()

    with assert_raises(ValueError):
        dist_mat[0, 0] = 1_000

    with assert_raises(ValueError):
        dur_mat[0, 0] = 1_000


def test_matrices_are_not_copies():
    """
    The matrices returned by ``distance_matrix()`` and ``duration_matrix()``
    offer views into data owned by the underlying ``ProblemData`` instance.
    There is no copying going on when accessing this data.
    """
    mat = np.array([[0, 0], [0, 0]])
    data = ProblemData(
        clients=[Client(x=0, y=0), Client(x=0, y=1)],
        vehicle_types=[VehicleType(1, 2)],
        distance_matrix=mat,
        duration_matrix=mat,
    )

    # Ownership is taken, so the memory that's referenced is not that of the
    # matrices that are passed into ProblemData's constructor.
    assert_(data.distance_matrix().base is not mat)
    assert_(data.duration_matrix().base is not mat)

    # Repeated calls should return matrices that reference the same base data,
    # implying nothing is copied: the memory is not owned by the matrix.
    dist1 = data.distance_matrix()
    dist2 = data.distance_matrix()
    assert_(not dist1.flags["OWNDATA"])
    assert_(dist1.base is dist2.base)

    dur1 = data.duration_matrix()
    dur2 = data.duration_matrix()
    assert_(not dur1.flags["OWNDATA"])
    assert_(dur1.base is dur2.base)

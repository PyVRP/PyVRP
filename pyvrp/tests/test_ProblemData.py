import sys

import numpy as np
import pytest
from numpy.random import default_rng
from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

from pyvrp import Client, ProblemData, VehicleType


@pytest.mark.parametrize(
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


@pytest.mark.parametrize(
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
        (1, 1, 1, 0, 1, 0, 0, 0),  # late < early
        (1, 1, 1, 0, -1, 0, 0, 0),  # negative early
        (1, 1, 0, -1, 0, 1, 0, 1),  # negative service duration
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


@pytest.mark.parametrize(
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
            clients=[],
            depots=[depot],
            vehicle_types=[VehicleType(1, 2)],
            distance_matrix=np.asarray([[0]], dtype=int),
            duration_matrix=np.asarray([[0]], dtype=int),
        )


def test_problem_data_raises_when_not_exactly_one_depot_is_provided():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    not exactly one depot is provided.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            depots=[],
            vehicle_types=[VehicleType(1, 2)],
            distance_matrix=np.asarray([[]], dtype=int),
            duration_matrix=np.asarray([[]], dtype=int),
        )

    # One depot should not raise.
    ProblemData(
        clients=[],
        depots=[Client(x=0, y=0)],
        vehicle_types=[VehicleType(1, 2)],
        distance_matrix=np.asarray([[0]]),
        duration_matrix=np.asarray([[0]]),
    )

    # But multiple (for now) do: PyVRP does not yet support multi-depot VRPs.
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            depots=[Client(x=0, y=0), Client(x=0, y=0)],
            vehicle_types=[VehicleType(1, 2)],
            distance_matrix=np.zeros((2, 2), dtype=int),
            duration_matrix=np.zeros((2, 2), dtype=int),
        )


@pytest.mark.parametrize(
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
    clients = [Client(x=0, y=0)]
    depots = [Client(x=0, y=0)]
    vehicle_types = [VehicleType(1, 2)]
    other_matrix = np.zeros((2, 2), dtype=int)  # this one's OK

    with assert_raises(ValueError):
        ProblemData(clients, depots, vehicle_types, matrix, other_matrix)

    with assert_raises(ValueError):
        ProblemData(clients, depots, vehicle_types, other_matrix, matrix)


def test_problem_data_replace_no_changes():
    """
    Tests that when using ``ProblemData.replace()`` without any arguments
    returns a new instance with different objects, but with the same values.
    """
    clients = [Client(x=0, y=0)]
    depots = [Client(x=0, y=0)]
    vehicle_types = [VehicleType(1, 2)]
    mat = np.zeros((2, 2), dtype=int)
    original = ProblemData(clients, depots, vehicle_types, mat, mat)

    new = original.replace()

    assert_(new is not original)

    for idx in range(new.num_clients):
        assert_(new.location(idx) is not original.location(idx))
        assert_equal(new.location(idx).x, original.location(idx).x)
        assert_equal(new.location(idx).y, original.location(idx).y)

    for idx in range(new.num_vehicle_types):
        new_veh_type = new.vehicle_type(idx)
        og_veh_type = original.vehicle_type(idx)

        assert_(new_veh_type is not og_veh_type)
        assert_equal(new_veh_type.capacity, og_veh_type.capacity)
        assert_equal(new_veh_type.num_available, og_veh_type.num_available)

    assert_(new.distance_matrix() is not original.distance_matrix())
    assert_allclose(new.distance_matrix(), original.distance_matrix())

    assert_(new.duration_matrix() is not original.duration_matrix())
    assert_allclose(new.duration_matrix(), original.duration_matrix())

    assert_equal(new.num_clients, original.num_clients)
    assert_equal(new.num_vehicle_types, original.num_vehicle_types)


def test_problem_data_replace_with_changes():
    """
    Tests that when calling ``ProblemData.replace()`` indeed replaces the
    data values with those passed to the method.
    """
    clients = [Client(x=0, y=0)]
    depots = [Client(x=0, y=0)]
    vehicle_types = [VehicleType(1, 2)]
    mat = np.zeros((2, 2), dtype=int)
    original = ProblemData(clients, depots, vehicle_types, mat, mat)

    # Let's replace the clients, vehicle types, and the distance matrix, each
    # with different values than in the original data. The duration matrix
    # is left unchanged.
    new = original.replace(
        clients=[Client(x=1, y=1)],
        vehicle_types=[VehicleType(3, 4), VehicleType(5, 6)],
        distance_matrix=np.ones((2, 2), dtype=int),
    )

    assert_(new is not original)
    assert_(new.location(1) is not original.location(1))
    assert_(new.location(1).x != original.location(1).x)
    assert_(new.location(1).y != original.location(1).y)

    for idx in range(original.num_vehicle_types):  # only compare first type
        new_veh_type = new.vehicle_type(idx)
        og_veh_type = original.vehicle_type(idx)

        assert_(new_veh_type is not og_veh_type)
        assert_(new_veh_type.capacity != og_veh_type.capacity)
        assert_(new_veh_type.num_available != og_veh_type.num_available)

    assert_(new.distance_matrix() is not original.distance_matrix())
    with assert_raises(AssertionError):
        assert_allclose(new.distance_matrix(), original.distance_matrix())

    assert_(new.duration_matrix() is not original.duration_matrix())
    assert_allclose(new.duration_matrix(), original.duration_matrix())

    assert_equal(new.num_clients, original.num_clients)
    assert_(new.num_vehicle_types != original.num_vehicle_types)


def test_problem_data_replace_raises_mismatched_argument_shapes():
    """
    Tests that a ValueError is raised when replacing data that result in
    mismatched shape between the clients and the distance/duration matrices.
    """
    clients = [Client(x=0, y=0)]
    depots = [Client(x=0, y=0)]
    vehicle_types = [VehicleType(1, 2)]
    mat = np.zeros((2, 2), dtype=int)
    data = ProblemData(clients, depots, vehicle_types, mat, mat)

    with assert_raises(ValueError):
        data.replace(clients=[])  # matrices are 2x2

    with assert_raises(ValueError):
        data.replace(distance_matrix=np.ones((3, 3), dtype=int))  # two clients

    with assert_raises(ValueError):
        data.replace(duration_matrix=np.ones((3, 3), dtype=int))  # two clients

    with assert_raises(ValueError):
        data.replace(
            clients=[Client(x=1, y=1)],
            distance_matrix=np.ones((3, 3), dtype=int),
            duration_matrix=np.ones((3, 3), dtype=int),
        )


def test_centroid(ok_small):
    """
    Tests the computation of the centroid of all clients in the data instance.
    """
    centroid = ok_small.centroid()
    x = [client.x for client in ok_small.clients()]
    y = [client.y for client in ok_small.clients()]

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
        clients=clients[1:],
        depots=clients[:1],
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
        clients=[],
        depots=[Client(x=0, y=0)],
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
        clients=[Client(x=0, y=1)],
        depots=[Client(x=0, y=0)],
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


@pytest.mark.parametrize(
    ("capacity", "num_available", "fixed_cost", "tw_early", "tw_late"),
    [
        (0, 0, 0, 0, 0),  # num_available must be positive
        (-1, 1, 1, 0, 0),  # capacity cannot be negative
        (-100, 1, 0, 0, 0),  # this is just wrong
        (1, 1, -1, 0, 0),  # fixed_cost cannot be negative
        (0, 1, -100, 0, 0),  # this is just wrong
        (0, 1, 0, None, 0),  # both shift time windows must be given (if set)
        (0, 1, 0, 0, None),  # both shift time windows must be given (if set)
        (0, 1, 0, 1, 0),  # early > late
        (0, 1, 0, -1, 0),  # negative early
    ],
)
def test_vehicle_type_raises_invalid_data(
    capacity: int,
    num_available: int,
    fixed_cost: int,
    tw_early: int,
    tw_late: int,
):
    """
    Tests that the vehicle type constructor raises when given invalid
    arguments.
    """
    with assert_raises(ValueError):
        VehicleType(capacity, num_available, fixed_cost, tw_early, tw_late)


def test_vehicle_type_init_max_duration_argument():
    """
    Tests valid and invalid edge cases of the max_duration argument.
    """
    with assert_raises(ValueError):
        VehicleType(1, 1, max_duration=-1)  # negative max_duration

    veh_type = VehicleType(1, 1, max_duration=0)  # valid edge case
    assert_allclose(veh_type.max_duration, 0)


def test_vehicle_type_does_not_raise_for_edge_cases():
    """
    The vehicle type constructor should allow the following edge case, of no
    capacity, costs, shift time windows, and just a single vehicle.
    """
    vehicle_type = VehicleType(
        capacity=0,
        num_available=1,
        fixed_cost=0,
        tw_early=0,
        tw_late=0,
    )

    assert_allclose(vehicle_type.capacity, 0)
    assert_equal(vehicle_type.num_available, 1)
    assert_equal(vehicle_type.fixed_cost, 0)
    assert_equal(vehicle_type.tw_early, 0)
    assert_equal(vehicle_type.tw_late, 0)


def test_vehicle_type_default_values():
    """
    Tests that the default values for costs and shift time windows are set
    correctly.
    """
    vehicle_type = VehicleType(capacity=0, num_available=1)
    assert_allclose(vehicle_type.fixed_cost, 0)
    assert_(vehicle_type.tw_early is None)
    assert_(vehicle_type.tw_late is None)

    # The C++ extensions can be compiled with support for either integer or
    # double precision. In each case, the default value for max_duration is
    # the largest representable value.
    if isinstance(vehicle_type.max_duration, int):
        assert_equal(vehicle_type.max_duration, np.iinfo(np.int32).max)
    else:
        assert_allclose(vehicle_type.max_duration, sys.float_info.max)


def test_vehicle_type_attribute_access():
    """
    Smoke test that checks all attributes are equal to the values they were
    given in the constructor's arguments.
    """
    vehicle_type = VehicleType(
        capacity=13,
        num_available=7,
        fixed_cost=3,
        tw_early=17,
        tw_late=19,
        max_duration=23,
    )

    assert_allclose(vehicle_type.capacity, 13)
    assert_equal(vehicle_type.num_available, 7)
    assert_allclose(vehicle_type.fixed_cost, 3)
    assert_allclose(vehicle_type.tw_early, 17)
    assert_allclose(vehicle_type.tw_late, 19)
    assert_allclose(vehicle_type.max_duration, 23)


@pytest.mark.parametrize("idx", [5, 6])
def test_location_raises_invalid_index(ok_small, idx: int):
    """
    Tests that calling location(idx) raises when the index is out of bounds.
    """
    assert_equal(ok_small.num_depots, 1)
    assert_equal(ok_small.num_clients, 4)

    with assert_raises(IndexError):
        ok_small.location(idx)

from typing import Optional

import numpy as np
import pytest
from numpy.random import default_rng
from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

from pyvrp import Client, ClientGroup, Depot, ProblemData, VehicleType


@pytest.mark.parametrize(
    (
        "x",
        "y",
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
        (1, 1, 1, 1, 1, 0, 1, 0, 0, True, None, "test name"),  # normal
        (1, 1, 1, 0, 0, 0, 1, 0, 0, True, None, "1234"),  # zero duration
        (1, 1, 0, 0, 1, 0, 1, 0, 0, True, None, "1,2,3,4"),  # zero delivery
        (1, 1, 1, 0, 1, 0, 0, 0, 0, True, None, ""),  # zero time windows
        (-1, -1, 1, 0, 1, 0, 1, 0, 0, True, None, ""),  # negative coordinates
        (1, 1, 1, 0, 1, 0, 1, 1, 0, True, None, ""),  # positive release time
        (0, 0, 1, 0, 1, 0, 1, 0, 1, True, None, ""),  # positive prize
        (0, 0, 1, 0, 1, 0, 1, 0, 1, False, None, ""),  # not required
        (0, 0, 1, 0, 1, 0, 1, 0, 1, False, 0, ""),  # group membership
    ],
)
def test_client_constructor_initialises_data_fields_correctly(
    x: int,
    y: int,
    delivery: int,
    pickup: int,
    service_duration: int,
    tw_early: int,
    tw_late: int,
    release_time: int,
    prize: int,
    required: bool,
    group: Optional[int],
    name: str,
):
    """
    Tests that the access properties return the data that was given to the
    Client's constructor.
    """
    client = Client(
        x=x,
        y=y,
        delivery=delivery,
        pickup=pickup,
        service_duration=service_duration,
        tw_early=tw_early,
        tw_late=tw_late,
        release_time=release_time,
        prize=prize,
        required=required,
        group=group,
        name=name,
    )

    assert_equal(client.x, x)
    assert_equal(client.y, y)
    assert_equal(client.delivery, delivery)
    assert_equal(client.pickup, pickup)
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
        "x",
        "y",
        "delivery",
        "pickup",
        "service",
        "tw_early",
        "tw_late",
        "release_time",
        "prize",
    ),
    [
        (1, 1, 1, 0, 0, 1, 0, 0, 0),  # late < early
        (1, 1, 1, 0, 0, -1, 0, 0, 0),  # negative early
        (1, 1, 0, 0, -1, 0, 1, 0, 1),  # negative service duration
        (1, 1, -1, 0, 1, 0, 1, 0, 0),  # negative delivery
        (1, 1, 0, -1, 1, 0, 1, 0, 0),  # negative pickup
        (1, 1, 0, 0, 0, 0, 1, -1, 0),  # negative release time
        (1, 1, 0, 0, 0, 0, 1, 2, 0),  # release time > late
        (1, 1, 1, 0, 1, 0, 1, 0, -1),  # negative prize
    ],
)
def test_raises_for_invalid_client_data(
    x: int,
    y: int,
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
            x,
            y,
            delivery,
            pickup,
            service,
            tw_early,
            tw_late,
            release_time,
            prize,
        )


@pytest.mark.parametrize(
    ("x", "y", "tw_early", "tw_late"),
    [
        (0, 0, 1, 0),  # tw_early > tw_late
        (0, 0, -1, 0),  # tw_early < 0
        (0, 0, 0, -1),  # tw_late < 0
    ],
)
def test_raises_for_invalid_depot_data(
    x: int,
    y: int,
    tw_early: int,
    tw_late: int,
):
    """
    Tests that an invalid depot configuration is not accepted.
    """
    with assert_raises(ValueError):
        Depot(x, y, tw_early, tw_late)


def test_problem_data_raises_when_no_depot_is_provided():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    no depots are provided.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            depots=[],
            vehicle_types=[VehicleType(2, capacity=1)],
            distance_matrix=np.asarray([[]], dtype=int),
            duration_matrix=np.asarray([[]], dtype=int),
        )

    # One (or more) depots should not raise.
    ProblemData(
        clients=[],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(2, capacity=1)],
        distance_matrix=np.asarray([[0]]),
        duration_matrix=np.asarray([[0]]),
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
    depots = [Depot(x=0, y=0)]
    vehicle_types = [VehicleType(2, capacity=1)]
    other_matrix = np.zeros((2, 2), dtype=int)  # this one's OK

    with assert_raises(ValueError):
        ProblemData(clients, depots, vehicle_types, matrix, other_matrix)

    with assert_raises(ValueError):
        ProblemData(clients, depots, vehicle_types, other_matrix, matrix)


@pytest.mark.parametrize(
    ("dist_mat", "dur_mat"),
    [
        (np.eye(2, dtype=int), np.zeros((2, 2), dtype=int)),  # distance diag
        (np.zeros((2, 2), dtype=int), np.eye(2, dtype=int)),  # duration diag
        (np.eye(2, dtype=int), np.eye(2, dtype=int)),  # both diags nonzero
    ],
)
def test_problem_data_raises_matrix_diagonal_nonzero(dist_mat, dur_mat):
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    the distance or duration matrix has a non-zero value on the diagonal.
    """
    clients = [Client(x=0, y=0)]
    depots = [Depot(x=0, y=0)]
    vehicle_types = [VehicleType(2, capacity=1)]

    with assert_raises(ValueError):
        ProblemData(clients, depots, vehicle_types, dist_mat, dur_mat)


def test_problem_data_replace_no_changes():
    """
    Tests that when using ``ProblemData.replace()`` without any arguments
    returns a new instance with different objects, but with the same values.
    """
    clients = [Client(x=0, y=0)]
    depots = [Depot(x=0, y=0)]
    vehicle_types = [VehicleType(2, capacity=1)]
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
    assert_equal(new.distance_matrix(), original.distance_matrix())

    assert_(new.duration_matrix() is not original.duration_matrix())
    assert_equal(new.duration_matrix(), original.duration_matrix())

    assert_equal(new.num_clients, original.num_clients)
    assert_equal(new.num_vehicle_types, original.num_vehicle_types)


def test_problem_data_replace_with_changes():
    """
    Tests that when calling ``ProblemData.replace()`` indeed replaces the
    data values with those passed to the method.
    """
    clients = [Client(x=0, y=0)]
    depots = [Depot(x=0, y=0)]
    vehicle_types = [VehicleType(2, capacity=1)]
    mat = np.zeros((2, 2), dtype=int)
    original = ProblemData(clients, depots, vehicle_types, mat, mat)

    # Let's replace the clients, vehicle types, and the distance matrix, each
    # with different values than in the original data. The duration matrix
    # is left unchanged.
    new = original.replace(
        clients=[Client(x=1, y=1)],
        vehicle_types=[VehicleType(3, 4), VehicleType(5, 6)],
        distance_matrix=np.where(np.eye(2), 0, 2),
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
        assert_equal(new.distance_matrix(), original.distance_matrix())

    assert_(new.duration_matrix() is not original.duration_matrix())
    assert_equal(new.duration_matrix(), original.duration_matrix())

    assert_equal(new.num_clients, original.num_clients)
    assert_(new.num_vehicle_types != original.num_vehicle_types)


def test_problem_data_replace_raises_mismatched_argument_shapes():
    """
    Tests that a ValueError is raised when replacing data that result in
    mismatched shape between the clients and the distance/duration matrices.
    """
    clients = [Client(x=0, y=0)]
    depots = [Depot(x=0, y=0)]
    vehicle_types = [VehicleType(2, capacity=1)]
    mat = np.zeros((2, 2), dtype=int)
    data = ProblemData(clients, depots, vehicle_types, mat, mat)

    with assert_raises(ValueError):
        data.replace(clients=[])  # matrices are 2x2

    with assert_raises(ValueError):
        data.replace(distance_matrix=np.where(np.eye(3), 0, 1))  # two clients

    with assert_raises(ValueError):
        data.replace(duration_matrix=np.where(np.eye(3), 0, 1))  # two clients

    with assert_raises(ValueError):
        data.replace(
            clients=[Client(x=1, y=1)],
            distance_matrix=np.where(np.eye(3), 0, 1),
            duration_matrix=np.where(np.eye(3), 0, 1),
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
    np.fill_diagonal(dist_mat, 0)
    np.fill_diagonal(dur_mat, 0)

    depot = Depot(x=0, y=0, tw_late=10)
    clients = [Client(x=0, y=0, tw_late=10) for _ in range(size - 1)]
    data = ProblemData(
        clients=clients,
        depots=[depot],
        vehicle_types=[VehicleType(2, capacity=1)],
        distance_matrix=dist_mat,
        duration_matrix=dur_mat,
    )

    assert_equal(data.distance_matrix(), dist_mat)
    assert_equal(data.duration_matrix(), dur_mat)

    for frm in range(size):
        for to in range(size):
            assert_equal(data.dist(frm, to), dist_mat[frm, to])
            assert_equal(data.duration(frm, to), dur_mat[frm, to])


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
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(2, capacity=1)],
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
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(2, capacity=1)],
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
    (
        "capacity",
        "num_available",
        "fixed_cost",
        "tw_early",
        "tw_late",
        "max_duration",
        "max_distance",
    ),
    [
        (0, 0, 0, 0, 0, 0, 0),  # num_available must be positive
        (-1, 1, 1, 0, 0, 0, 0),  # capacity cannot be negative
        (-100, 1, 0, 0, 0, 0, 0),  # this is just wrong
        (1, 1, -1, 0, 0, 0, 0),  # fixed_cost cannot be negative
        (0, 1, -100, 0, 0, 0, 0),  # this is just wrong
        (0, 1, 0, 1, 0, 0, 0),  # early > late
        (0, 1, 0, -1, 0, 0, 0),  # negative early
        (0, 1, 0, 0, -1, 0, 0),  # negative
        (0, 1, 0, 0, 0, -1, 0),  # negative max_duration
        (0, 1, 0, 0, 0, 0, -1),  # negative max_distance
    ],
)
def test_vehicle_type_raises_invalid_data(
    capacity: int,
    num_available: int,
    fixed_cost: int,
    tw_early: int,
    tw_late: int,
    max_duration: int,
    max_distance: int,
):
    """
    Tests that the vehicle type constructor raises when given invalid
    arguments.
    """
    with assert_raises(ValueError):
        VehicleType(
            num_available,
            capacity,
            0,
            fixed_cost,
            tw_early,
            tw_late,
            max_duration,
            max_distance,
        )


def test_vehicle_type_does_not_raise_for_all_zero_edge_case():
    """
    The vehicle type constructor should allow the following edge case where all
    data has been zeroed out.
    """
    vehicle_type = VehicleType(
        num_available=1,
        depot=0,
        capacity=0,
        fixed_cost=0,
        tw_early=0,
        tw_late=0,
        max_duration=0,
        max_distance=0,
    )

    assert_equal(vehicle_type.num_available, 1)
    assert_equal(vehicle_type.depot, 0)
    assert_equal(vehicle_type.capacity, 0)
    assert_equal(vehicle_type.fixed_cost, 0)
    assert_equal(vehicle_type.tw_early, 0)
    assert_equal(vehicle_type.tw_late, 0)
    assert_equal(vehicle_type.max_duration, 0)
    assert_equal(vehicle_type.max_distance, 0)


def test_vehicle_type_default_values():
    """
    Tests that the default values for costs and shift time windows are set
    correctly.
    """
    vehicle_type = VehicleType()
    assert_equal(vehicle_type.num_available, 1)
    assert_equal(vehicle_type.depot, 0)
    assert_equal(vehicle_type.capacity, 0)
    assert_equal(vehicle_type.fixed_cost, 0)
    assert_equal(vehicle_type.tw_early, 0)
    assert_equal(vehicle_type.name, "")

    # The default value for the following fields is the largest representable
    # integral value.
    assert_equal(vehicle_type.tw_late, np.iinfo(np.int64).max)
    assert_equal(vehicle_type.max_duration, np.iinfo(np.int64).max)
    assert_equal(vehicle_type.max_distance, np.iinfo(np.int64).max)


def test_vehicle_type_attribute_access():
    """
    Smoke test that checks all attributes are equal to the values they were
    given in the constructor's arguments.
    """
    vehicle_type = VehicleType(
        num_available=7,
        depot=29,
        capacity=13,
        fixed_cost=3,
        tw_early=17,
        tw_late=19,
        max_duration=23,
        max_distance=29,
        name="vehicle_type name",
    )

    assert_equal(vehicle_type.num_available, 7)
    assert_equal(vehicle_type.depot, 29)
    assert_equal(vehicle_type.capacity, 13)
    assert_equal(vehicle_type.fixed_cost, 3)
    assert_equal(vehicle_type.tw_early, 17)
    assert_equal(vehicle_type.tw_late, 19)
    assert_equal(vehicle_type.max_duration, 23)
    assert_equal(vehicle_type.max_distance, 29)

    assert_equal(vehicle_type.name, "vehicle_type name")
    assert_equal(str(vehicle_type), "vehicle_type name")


@pytest.mark.parametrize("idx", [5, 6])
def test_location_raises_invalid_index(ok_small, idx: int):
    """
    Tests that calling location(idx) raises when the index is out of bounds.
    """
    assert_equal(ok_small.num_depots, 1)
    assert_equal(ok_small.num_clients, 4)

    with assert_raises(IndexError):
        ok_small.location(idx)


@pytest.mark.parametrize(
    ("depot", "should_raise"),
    [
        (0, False),  # correct index - should not raise
        (1, True),  # index is the number of depots: too large; should raise
        (2, True),  # index is too large; should raise
    ],
)
def test_raises_invalid_vehicle_depot_indices(
    ok_small, depot: int, should_raise: bool
):
    """
    Tests that setting the depot index on a VehicleType to a value that's not
    correct raises when replacing vehicles (and, by extension, when
    constructing a ProblemData instance).
    """
    assert_equal(ok_small.num_depots, 1)

    if not should_raise:
        ok_small.replace(vehicle_types=[VehicleType(depot=depot)])
        return

    with assert_raises(IndexError):
        ok_small.replace(vehicle_types=[VehicleType(depot=depot)])


def test_raises_empty_group():
    """
    Tests that passing an empty client group raises a ValueError.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            depots=[Depot(1, 1)],
            vehicle_types=[VehicleType()],
            distance_matrix=[[0]],
            duration_matrix=[[0]],
            groups=[ClientGroup()],  # empty group - this should raise
        )


@pytest.mark.parametrize(
    ("groups", "index"),
    [
        ([], 0),  # index 0, but there are no groups
        ([ClientGroup([1])], 1),  # there is one group, but index is 1
    ],
)
def test_raises_invalid_client_group_indices(
    groups: list[ClientGroup], index: int
):
    """
    Tests that setting the clients in a mutually exclusive group to values that
    are not valid indices raises, and that setting a group index on a client to
    a value that's out of range likewise raises.
    """
    with assert_raises(IndexError):
        ProblemData(
            clients=[Client(1, 1, required=False, group=index)],
            depots=[Depot(1, 1)],
            vehicle_types=[VehicleType()],
            distance_matrix=np.zeros((2, 2)),
            duration_matrix=np.zeros((2, 2)),
            groups=groups,
        )


@pytest.mark.parametrize(
    "groups", [[ClientGroup([0, 1])], [ClientGroup([1, 2])]]
)
def test_raises_invalid_group_client_indices(groups: list[ClientGroup]):
    """
    Tests that groups with client indices that are either depots or outside the
    range of client locations results in an IndexError.
    """
    with assert_raises(IndexError):
        ProblemData(
            clients=[Client(1, 1, required=False, group=0)],
            depots=[Depot(1, 1)],
            vehicle_types=[VehicleType()],
            distance_matrix=np.zeros((2, 2)),
            duration_matrix=np.zeros((2, 2)),
            groups=groups,
        )


def test_raises_wrong_mutual_group_referencing():
    """
    Groups should reference the clients in the group, and vice versa, the
    client should reference the group. If this is not done correctly, a
    ValueError should be thrown.
    """
    with assert_raises(ValueError):
        ProblemData(
            # The client references the first group, which does not contain the
            # client. That should raise.
            clients=[
                Client(1, 1, required=False, group=0),
                Client(2, 2, required=False, group=0),
            ],
            depots=[Depot(1, 1)],
            vehicle_types=[VehicleType()],
            distance_matrix=np.zeros((3, 3)),
            duration_matrix=np.zeros((3, 3)),
            groups=[ClientGroup([2])],
        )

    with assert_raises(ValueError):
        ProblemData(
            clients=[Client(1, 1), Client(2, 2)],
            depots=[Depot(1, 1)],
            vehicle_types=[VehicleType()],
            distance_matrix=np.zeros((3, 3)),
            duration_matrix=np.zeros((3, 3)),
            # Group references a client that is not in the group. That should
            # raise as well.
            groups=[ClientGroup([1])],
        )


def test_raises_for_required_mutually_exclusive_group_membership():
    """
    Tests that required clients cannot be part of mutually exclusive groups.
    """
    with assert_raises(ValueError):
        # A client cannot be part of a mutually exclusive group and also be a
        # required visit, as that defeats the entire point of a mutually
        # exclusive group.
        ProblemData(
            clients=[Client(1, 1, required=True, group=0)],
            depots=[Depot(1, 1)],
            vehicle_types=[VehicleType()],
            distance_matrix=np.zeros((2, 2)),
            duration_matrix=np.zeros((2, 2)),
            groups=[ClientGroup([1])],
        )


def test_client_group_raises_duplicate_clients():
    """
    Tests that adding the same client to a group more than once raises.
    """
    with assert_raises(ValueError):
        ClientGroup([1, 1])

    group = ClientGroup()
    group.add_client(1)  # this should be OK

    with assert_raises(ValueError):
        group.add_client(1)  # but adding the client a second time is not


def test_replacing_client_groups(ok_small):
    """
    Tests that replacing mutually exclusive client groups works well.
    """
    assert_equal(ok_small.num_groups, 0)
    assert_equal(ok_small.groups(), [])

    # Let's add the first client to a group, and define a new data instance
    # that has a mutually exclusive group.
    clients = ok_small.clients()
    clients[0] = Client(1, 1, required=False, group=0)
    data = ok_small.replace(clients=clients, groups=[ClientGroup([1])])

    # There should now be a single client group (at index 0) that has the first
    # client as its only member.
    assert_equal(data.num_groups, 1)
    assert_equal(data.group(0).clients, [1])

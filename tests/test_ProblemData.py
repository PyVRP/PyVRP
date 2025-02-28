import pickle

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
    group: int | None,
    name: str,
):
    """
    Tests that the access properties return the data that was given to the
    Client's constructor.
    """
    client = Client(
        x=x,
        y=y,
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

    assert_equal(client.x, x)
    assert_equal(client.y, y)
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
            [delivery],
            [pickup],
            service,
            tw_early,
            tw_late,
            release_time,
            prize,
        )


@pytest.mark.parametrize(
    ("x", "y", "tw_early", "tw_late", "service_duration"),
    [
        (0, 0, 1, 0, 0),  # tw_early > tw_late
        (0, 0, -1, 0, 0),  # tw_early < 0
        (0, 0, 0, -1, 0),  # tw_late < 0
        (0, 0, 0, 0, -1),  # service_duration < 0
    ],
)
def test_raises_for_invalid_depot_data(
    x: int,
    y: int,
    tw_early: int,
    tw_late: int,
    service_duration: int,
):
    """
    Tests that an invalid depot configuration is not accepted.
    """
    with assert_raises(ValueError):
        Depot(x, y, service_duration, tw_early, tw_late)


def test_depot_initialises_data_correctly():
    """
    Tests that the depot constructor correctly initialises its member data, and
    ensures the data is accessible from Python.
    """
    depot = Depot(
        x=1,
        y=2,
        service_duration=3,
        tw_early=5,
        tw_late=7,
        name="test",
    )

    assert_equal(depot.x, 1)
    assert_equal(depot.y, 2)
    assert_equal(depot.service_duration, 3)
    assert_equal(depot.tw_early, 5)
    assert_equal(depot.tw_late, 7)
    assert_equal(depot.name, "test")


def test_problem_data_raises_when_no_depot_is_provided():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    no depots are provided.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            depots=[],
            vehicle_types=[VehicleType()],
            distance_matrices=[np.asarray([[]], dtype=int)],
            duration_matrices=[np.asarray([[]], dtype=int)],
        )

    # One (or more) depots should not raise.
    ProblemData(
        clients=[],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.asarray([[0]])],
        duration_matrices=[np.asarray([[0]])],
    )


def test_problem_data_raises_when_no_vehicle_type_is_provided():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    no vehicle types are provided.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            depots=[Depot(x=0, y=0)],
            vehicle_types=[],
            distance_matrices=[np.asarray([[0]])],
            duration_matrices=[np.asarray([[0]])],
        )

    # One (or more) vehicle types should not raise.
    ProblemData(
        clients=[],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.asarray([[0]])],
        duration_matrices=[np.asarray([[0]])],
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
    vehicle_types = [VehicleType()]
    other_matrix = np.zeros((2, 2), dtype=int)  # this one's OK

    with assert_raises(ValueError):
        ProblemData(clients, depots, vehicle_types, [matrix], [other_matrix])

    with assert_raises(ValueError):
        ProblemData(clients, depots, vehicle_types, [other_matrix], [matrix])


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
    vehicle_types = [VehicleType()]

    with assert_raises(ValueError):
        ProblemData(clients, depots, vehicle_types, [dist_mat], [dur_mat])


def test_problem_data_replace_no_changes():
    """
    Tests that when using ``ProblemData.replace()`` without any arguments
    returns a new instance with different objects, but with the same values.
    """
    clients = [Client(x=0, y=0)]
    depots = [Depot(x=0, y=0)]
    vehicle_types = [VehicleType()]
    mat = np.zeros((2, 2), dtype=int)
    original = ProblemData(clients, depots, vehicle_types, [mat], [mat])

    new = original.replace()

    assert_(new is not original)

    for idx in range(new.num_clients):
        assert_(new.location(idx) is not original.location(idx))
        assert_equal(new.location(idx).x, original.location(idx).x)
        assert_equal(new.location(idx).y, original.location(idx).y)

    for idx in range(new.num_vehicle_types):
        new_veh_type = new.vehicle_type(idx)
        orig_veh_type = original.vehicle_type(idx)

        assert_(new_veh_type is not orig_veh_type)
        assert_equal(new_veh_type.capacity, orig_veh_type.capacity)
        assert_equal(new_veh_type.num_available, orig_veh_type.num_available)

    new_dist = new.distance_matrix(profile=0)
    orig_dist = original.distance_matrix(profile=0)
    assert_(new_dist is not orig_dist)
    assert_equal(new_dist, orig_dist)

    new_dur = new.duration_matrix(profile=0)
    orig_dur = original.duration_matrix(profile=0)
    assert_(new_dur is not orig_dur)
    assert_equal(new_dur, orig_dur)

    assert_equal(new.num_clients, original.num_clients)
    assert_equal(new.num_vehicle_types, original.num_vehicle_types)
    assert_equal(new.num_load_dimensions, original.num_load_dimensions)


def test_problem_data_replace_with_changes():
    """
    Tests that when calling ``ProblemData.replace()`` indeed replaces the
    data values with those passed to the method.
    """
    clients = [Client(x=0, y=0, delivery=[0])]
    depots = [Depot(x=0, y=0)]
    vehicle_types = [VehicleType(2, capacity=[1])]
    mat = np.zeros((2, 2), dtype=int)
    original = ProblemData(clients, depots, vehicle_types, [mat], [mat])

    # Let's replace the clients, vehicle types, and the distance matrix, each
    # with different values than in the original data. The duration matrix
    # is left unchanged.
    new = original.replace(
        clients=[Client(x=1, y=1, delivery=[0])],
        vehicle_types=[VehicleType(3, [4]), VehicleType(5, [6])],
        distance_matrices=[np.where(np.eye(2), 0, 2)],
    )

    assert_(new is not original)
    assert_(new.location(1) is not original.location(1))
    assert_(new.location(1).x != original.location(1).x)
    assert_(new.location(1).y != original.location(1).y)

    for idx in range(original.num_vehicle_types):  # only compare first type
        new_veh_type = new.vehicle_type(idx)
        orig_veh_type = original.vehicle_type(idx)

        assert_(new_veh_type is not orig_veh_type)
        assert_(new_veh_type.capacity != orig_veh_type.capacity)
        assert_(new_veh_type.num_available != orig_veh_type.num_available)

    assert_(new.distance_matrix(0) is not original.distance_matrix(0))
    with assert_raises(AssertionError):
        assert_equal(new.distance_matrix(0), original.distance_matrix(0))

    assert_(new.duration_matrix(0) is not original.duration_matrix(0))
    assert_equal(new.duration_matrix(0), original.duration_matrix(0))

    assert_equal(new.num_clients, original.num_clients)
    assert_(new.num_vehicle_types != original.num_vehicle_types)


def test_problem_data_replace_raises_mismatched_argument_shapes():
    """
    Tests that a ValueError is raised when replacing data that result in
    mismatched shape between the clients and the distance/duration matrices.
    """
    clients = [Client(x=0, y=0)]
    depots = [Depot(x=0, y=0)]
    vehicle_types = [VehicleType(2)]
    mat = np.zeros((2, 2), dtype=int)
    data = ProblemData(clients, depots, vehicle_types, [mat], [mat])

    with assert_raises(ValueError):  # matrices are 2x2
        data.replace(clients=[])

    with assert_raises(ValueError):  # two clients
        data.replace(distance_matrices=[np.where(np.eye(3), 0, 1)])

    with assert_raises(ValueError):  # two clients
        data.replace(duration_matrices=[np.where(np.eye(3), 0, 1)])

    with assert_raises(ValueError):
        data.replace(
            clients=[Client(x=1, y=1)],
            distance_matrices=[np.where(np.eye(3), 0, 1)],
            duration_matrices=[np.where(np.eye(3), 0, 1)],
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
    Tests that the ``duration_matrix()`` and ``distance_matrix()`` methods
    correctly return the underlying data matrices.
    """
    gen = default_rng(seed=42)
    size = 6

    dist_mat = gen.integers(500, size=(size, size))
    dur_mat = gen.integers(500, size=(size, size))
    np.fill_diagonal(dist_mat, 0)
    np.fill_diagonal(dur_mat, 0)

    data = ProblemData(
        clients=[Client(x=0, y=0, tw_late=10) for _ in range(size - 1)],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[VehicleType(2)],
        distance_matrices=[dist_mat],
        duration_matrices=[dur_mat],
    )

    assert_equal(data.distance_matrix(profile=0), dist_mat)
    assert_equal(data.duration_matrix(profile=0), dur_mat)


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
        vehicle_types=[VehicleType(2)],
        distance_matrices=[np.array([[0]])],
        duration_matrices=[np.array([[0]])],
    )

    dist_mat = data.distance_matrix(profile=0)
    dur_mat = data.duration_matrix(profile=0)

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
        vehicle_types=[VehicleType(2)],
        distance_matrices=[mat],
        duration_matrices=[mat],
    )

    # Ownership is taken, so the memory that's referenced is not that of the
    # matrices that are passed into ProblemData's constructor.
    assert_(data.distance_matrix(profile=0).base is not mat)
    assert_(data.duration_matrix(profile=0).base is not mat)

    # Repeated calls should return matrices that reference the same base data,
    # implying nothing is copied: the memory is not owned by the matrix.
    dist1 = data.distance_matrix(profile=0)
    dist2 = data.distance_matrix(profile=0)
    assert_(not dist1.flags["OWNDATA"])
    assert_(dist1.base is dist2.base)

    dur1 = data.duration_matrix(profile=0)
    dur2 = data.duration_matrix(profile=0)
    assert_(not dur1.flags["OWNDATA"])
    assert_(dur1.base is dur2.base)


@pytest.mark.parametrize(
    (
        "capacity",
        "num_available",
        "tw_early",
        "tw_late",
        "max_duration",
        "max_distance",
        "fixed_cost",
        "unit_distance_cost",
        "unit_duration_cost",
        "start_late",
        "initial_load",
    ),
    [
        (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),  # num_available must be positive
        (-1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0),  # capacity cannot be negative
        (-100, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0),  # this is just wrong
        (0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0),  # early > start late
        (0, 1, 1, 1, 0, 0, 0, 0, 0, 2, 0),  # start late > late
        (0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 0),  # negative early
        (0, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0),  # negative late
        (0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0),  # negative max_duration
        (0, 1, 0, 0, 0, -1, 0, 0, 0, 0, 0),  # negative max_distance
        (0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0),  # negative fixed_cost
        (0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 0),  # negative unit_distance_cost
        (0, 1, 0, 0, 0, 0, 0, 0, -1, 0, 0),  # negative unit_duration_cost
        (0, 1, 0, 0, 0, 0, 0, 0, 0, -1, 0),  # negative start late
        (0, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1),  # negative initial load
        (0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2),  # initial load exceeds capacity
    ],
)
def test_vehicle_type_raises_invalid_data(
    capacity: int,
    num_available: int,
    tw_early: int,
    tw_late: int,
    max_duration: int,
    max_distance: int,
    fixed_cost: int,
    unit_distance_cost: int,
    unit_duration_cost: int,
    start_late: int,
    initial_load: int,
):
    """
    Tests that the vehicle type constructor raises when given invalid
    arguments.
    """
    with assert_raises(ValueError):
        VehicleType(
            num_available=num_available,
            capacity=[capacity],
            fixed_cost=fixed_cost,
            tw_early=tw_early,
            tw_late=tw_late,
            max_duration=max_duration,
            max_distance=max_distance,
            unit_distance_cost=unit_distance_cost,
            unit_duration_cost=unit_duration_cost,
            start_late=start_late,
            initial_load=[initial_load],
        )


def test_vehicle_type_does_not_raise_for_all_zero_edge_case():
    """
    The vehicle type constructor should allow the following edge case where all
    data has been zeroed out.
    """
    vehicle_type = VehicleType(
        num_available=1,
        capacity=[],
        start_depot=0,
        end_depot=0,
        fixed_cost=0,
        tw_early=0,
        tw_late=0,
        max_duration=0,
        max_distance=0,
        unit_distance_cost=0,
        unit_duration_cost=0,
        start_late=0,
    )

    assert_equal(vehicle_type.num_available, 1)
    assert_equal(vehicle_type.start_depot, 0)
    assert_equal(vehicle_type.end_depot, 0)
    assert_equal(vehicle_type.capacity, [])
    assert_equal(vehicle_type.fixed_cost, 0)
    assert_equal(vehicle_type.tw_early, 0)
    assert_equal(vehicle_type.tw_late, 0)
    assert_equal(vehicle_type.max_duration, 0)
    assert_equal(vehicle_type.max_distance, 0)
    assert_equal(vehicle_type.unit_distance_cost, 0)
    assert_equal(vehicle_type.unit_duration_cost, 0)
    assert_equal(vehicle_type.start_late, 0)


def test_vehicle_type_default_values():
    """
    Tests that the default values for costs and shift time windows are set
    correctly.
    """
    vehicle_type = VehicleType()
    assert_equal(vehicle_type.num_available, 1)
    assert_equal(vehicle_type.start_depot, 0)
    assert_equal(vehicle_type.end_depot, 0)
    assert_equal(vehicle_type.capacity, [])
    assert_equal(vehicle_type.fixed_cost, 0)
    assert_equal(vehicle_type.tw_early, 0)
    assert_equal(vehicle_type.unit_distance_cost, 1)
    assert_equal(vehicle_type.unit_duration_cost, 0)
    assert_equal(vehicle_type.name, "")

    # The default value for the following fields is the largest representable
    # integral value.
    assert_equal(vehicle_type.tw_late, np.iinfo(np.int64).max)
    assert_equal(vehicle_type.max_duration, np.iinfo(np.int64).max)
    assert_equal(vehicle_type.max_distance, np.iinfo(np.int64).max)

    # The default value for start_late is the value of tw_late.
    assert_equal(vehicle_type.start_late, vehicle_type.tw_late)


def test_vehicle_type_attribute_access():
    """
    Smoke test that checks all attributes are equal to the values they were
    given in the constructor's arguments.
    """
    vehicle_type = VehicleType(
        num_available=7,
        start_depot=29,
        end_depot=43,
        capacity=[13],
        fixed_cost=3,
        tw_early=17,
        tw_late=19,
        max_duration=23,
        max_distance=31,
        unit_distance_cost=37,
        unit_duration_cost=41,
        start_late=18,
        name="vehicle_type name",
    )

    assert_equal(vehicle_type.num_available, 7)
    assert_equal(vehicle_type.start_depot, 29)
    assert_equal(vehicle_type.end_depot, 43)
    assert_equal(vehicle_type.capacity, [13])
    assert_equal(vehicle_type.fixed_cost, 3)
    assert_equal(vehicle_type.tw_early, 17)
    assert_equal(vehicle_type.tw_late, 19)
    assert_equal(vehicle_type.max_duration, 23)
    assert_equal(vehicle_type.max_distance, 31)
    assert_equal(vehicle_type.unit_distance_cost, 37)
    assert_equal(vehicle_type.unit_duration_cost, 41)
    assert_equal(vehicle_type.start_late, 18)

    assert_equal(vehicle_type.name, "vehicle_type name")
    assert_equal(str(vehicle_type), "vehicle_type name")


def test_vehicle_type_replace():
    """
    Tests that calling replace() on a VehicleType functions correctly.
    """
    vehicle_type = VehicleType(num_available=7, capacity=[10], name="test")
    assert_equal(vehicle_type.num_available, 7)
    assert_equal(vehicle_type.capacity, [10])
    assert_equal(vehicle_type.name, "test")

    # Replacing the number of available vehicles and name should be reflected
    # in the returned vehicle type, but any other values should remain the same
    # as the original. In particular, capacity should not be changed.
    new = vehicle_type.replace(num_available=5, name="new")
    assert_equal(new.num_available, 5)
    assert_equal(new.capacity, [10])
    assert_equal(new.name, "new")


def test_vehicle_type_multiple_capacities():
    """
    Tests that vehicle types correctly handle multiple capacities.
    """
    vehicle_type = VehicleType(capacity=[998, 37], num_available=10)
    assert_equal(vehicle_type.num_available, 10)
    assert_equal(vehicle_type.capacity, [998, 37])


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
    ("start_depot", "end_depot", "should_raise"),
    [
        (0, 0, False),  # correct; index smaller than number of depots
        (1, 0, True),  # index is too large; same as number of depots
        (0, 1, True),  # index is too large; same as number of depots
        (2, 0, True),  # index is too large; bigger than number of depots
        (0, 2, True),  # index is too large; bigger than number of depots
    ],
)
def test_raises_invalid_vehicle_depot_indices(
    ok_small, start_depot: int, end_depot: int, should_raise: bool
):
    """
    Tests that setting the depot index on a VehicleType to a value that's not
    correct raises when replacing vehicles (and, by extension, when
    constructing a ProblemData instance).
    """
    assert_equal(ok_small.num_depots, 1)
    vehicle_type = ok_small.vehicle_type(0)
    new_type = vehicle_type.replace(
        start_depot=start_depot,
        end_depot=end_depot,
    )

    if not should_raise:
        ok_small.replace(vehicle_types=[new_type])
        return

    with assert_raises(IndexError):
        ok_small.replace(vehicle_types=[new_type])


def test_raises_invalid_vehicle_profile_index(ok_small):
    """
    Tests that setting the profile index on a VehicleType to a value that's
    outside the range of available profiles raises.
    """
    assert_equal(ok_small.num_profiles, 1)

    with assert_raises(IndexError):
        ok_small.replace(vehicle_types=[VehicleType(capacity=[10], profile=1)])


@pytest.mark.parametrize(
    ("distances", "durations"), [([], []), ([], None), (None, [])]
)
def test_raises_no_profiles(ok_small, distances, durations):
    """
    Tests that passing no profiles (i.e., no distance and duration matrices)
    raises an error.
    """
    with assert_raises((ValueError, IndexError)):
        ok_small.replace(
            distance_matrices=distances,
            duration_matrices=durations,
        )


def test_raises_inconsistent_profiles(ok_small):
    """
    Tests that passing an inconsistent number of distance and duration matrices
    raises an error.
    """
    with assert_raises(ValueError):
        ok_small.replace(distance_matrices=ok_small.distance_matrices() * 2)


def test_raises_empty_group():
    """
    Tests that passing an empty client group raises a ValueError.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            depots=[Depot(1, 1)],
            vehicle_types=[VehicleType()],
            distance_matrices=[[[0]]],
            duration_matrices=[[[0]]],
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
            distance_matrices=[np.zeros((2, 2))],
            duration_matrices=[np.zeros((2, 2))],
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
            distance_matrices=[np.zeros((2, 2))],
            duration_matrices=[np.zeros((2, 2))],
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
            distance_matrices=[np.zeros((3, 3))],
            duration_matrices=[np.zeros((3, 3))],
            groups=[ClientGroup([2])],
        )

    with assert_raises(ValueError):
        ProblemData(
            clients=[Client(1, 1), Client(2, 2)],
            depots=[Depot(1, 1)],
            vehicle_types=[VehicleType()],
            distance_matrices=[np.zeros((3, 3))],
            duration_matrices=[np.zeros((3, 3))],
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
            distance_matrices=[np.zeros((2, 2))],
            duration_matrices=[np.zeros((2, 2))],
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
    clients[0] = Client(1, 1, delivery=[1], required=False, group=0)
    data = ok_small.replace(clients=clients, groups=[ClientGroup([1])])

    # There should now be a single client group (at index 0) that has the first
    # client as its only member.
    assert_equal(data.num_groups, 1)
    assert_equal(data.group(0).clients, [1])


def test_client_eq():
    """
    Tests the client's equality operator.
    """
    client1 = Client(x=0, y=0, delivery=[1], pickup=[2], tw_late=3, group=0)
    client2 = Client(x=0, y=0, delivery=[1], pickup=[2], tw_late=3, group=1)
    assert_(client1 != client2)

    # This client is equivalent to client1.
    client3 = Client(x=0, y=0, delivery=[1], pickup=[2], tw_late=3, group=0)
    assert_(client1 == client3)
    assert_(client3 == client3)

    # And some things that are not clients.
    assert_(client1 != "text")
    assert_(client1 != 1)


def test_depot_eq():
    """
    Tests the depot's equality operator.
    """
    depot1 = Depot(x=0, y=0)
    depot2 = Depot(x=0, y=1)
    assert_(depot1 != depot2)

    # This depot is equivalent to depot1.
    depot3 = Depot(x=0, y=0)
    assert_(depot1 == depot3)
    assert_(depot3 == depot3)

    # And some things that are not depots.
    assert_(depot1 != "text")
    assert_(depot1 != 3)

    depot4 = Depot(x=0, y=0, name="test")
    assert_(depot1 != depot4)


def test_vehicle_type_eq():
    """
    Tests the vehicle type's equality operator.
    """
    veh_type1 = VehicleType(num_available=3, profile=0)
    veh_type2 = VehicleType(num_available=3, profile=1)
    assert_(veh_type1 != veh_type2)

    # This vehicle type is equivalent to veh_type1.
    veh_type3 = VehicleType(num_available=3, profile=0)
    assert_(veh_type1 == veh_type3)

    # And some things that are not vehicle types.
    assert_(veh_type1 != "text")
    assert_(veh_type1 != 5)


def test_eq_checks_names():
    """
    Tests that the equality operators on named objects also considers the name
    when determining equality.
    """
    x, y = 0, 0
    assert_(Client(x, y, name="1") != Client(x, y, name="2"))
    assert_(Depot(x, y, name="1") != Depot(x, y, name="2"))
    assert_(VehicleType(name="1") != VehicleType(name="2"))


@pytest.mark.parametrize("cls", (Client, Depot))
def test_pickle_locations(cls):
    """
    Tests that client and depot locations can be serialised and unserialised.
    """
    before_pickle = cls(x=0, y=1, name="test")
    bytes = pickle.dumps(before_pickle)
    assert_equal(pickle.loads(bytes), before_pickle)


def test_pickle_client_group():
    """
    Tests that client groups can be serialised and unserialised.
    """
    before_pickle = ClientGroup(clients=[1, 2, 3], required=False)
    bytes = pickle.dumps(before_pickle)
    assert_equal(pickle.loads(bytes), before_pickle)


def test_pickle_vehicle_type():
    """
    Tests that vehicle types can be serialised and unserialised.
    """
    before_pickle = VehicleType(num_available=12, capacity=[3], name="test123")
    bytes = pickle.dumps(before_pickle)
    assert_equal(pickle.loads(bytes), before_pickle)


def test_pickle_data(ok_small, rc208):
    """
    Tests that problem data instances can be serialised and unserialised.
    """
    bytes = pickle.dumps(ok_small)
    assert_equal(pickle.loads(bytes), ok_small)

    bytes = pickle.dumps(rc208)
    assert_equal(pickle.loads(bytes), rc208)


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
    client = Client(x=0, y=1, delivery=delivery, pickup=pickup)
    assert_equal(client.delivery, exp_delivery)
    assert_equal(client.pickup, exp_pickup)


@pytest.mark.parametrize(
    ("capacity", "initial_load", "exp_capacity", "exp_initial_load"),
    [
        ([0], [0], [0], [0]),
        ([0], [0, 0, 0], [0, 0, 0], [0, 0, 0]),
        ([0, 1, 2], [0], [0, 1, 2], [0, 0, 0]),
        ([1, 2], [1], [1, 2], [1, 0]),
        ([], [], [], []),
    ],
)
def test_vehicle_load_dimensions_are_padded_with_zeroes(
    capacity: list[int],
    initial_load: list[int],
    exp_capacity: list[int],
    exp_initial_load: list[int],
):
    """
    Tests that any missing load dimensions for the capacity and initial_load
    VehicleType arguments are padded with zeroes.
    """
    vehicle_type = VehicleType(capacity=capacity, initial_load=initial_load)
    assert_equal(vehicle_type.capacity, exp_capacity)
    assert_equal(vehicle_type.initial_load, exp_initial_load)


def test_problem_data_raises_when_pickup_and_delivery_dimensions_differ():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    clients are provided with different dimensions for delivery and pickup.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[
                Client(x=0, y=0, delivery=[1, 2], pickup=[1, 2]),
                Client(x=1, y=1, delivery=[1, 2, 3], pickup=[1, 2, 3]),
            ],
            depots=[Depot(x=0, y=0)],
            vehicle_types=[],
            distance_matrices=[np.zeros((3, 3), dtype=int)],
            duration_matrices=[np.zeros((3, 3), dtype=int)],
        )


def test_problem_data_raises_when_capacity_dimensions_differ():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    vehicle types are provided with different dimensions for capacity.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            depots=[Depot(x=0, y=0)],
            vehicle_types=[
                VehicleType(2, capacity=[1, 2]),
                VehicleType(2, capacity=[1, 2, 3]),
            ],
            distance_matrices=[np.zeros((1, 1), dtype=int)],
            duration_matrices=[np.zeros((1, 1), dtype=int)],
        )


def test_problem_data_raises_when_pickup_delivery_capacity_dimensions_differ():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    client dimensions for pickup and delivery do not match the vehicle type
    dimensions for capacity.
    """
    with assert_raises(ValueError):
        ProblemData(
            clients=[
                Client(x=0, y=0, delivery=[1, 2], pickup=[1, 2]),
                Client(x=1, y=1, delivery=[1, 2], pickup=[1, 2]),
            ],
            depots=[Depot(x=0, y=0)],
            vehicle_types=[VehicleType(2, capacity=[1, 2, 3])],
            distance_matrices=[np.zeros((3, 3), dtype=int)],
            duration_matrices=[np.zeros((3, 3), dtype=int)],
        )


def test_problem_data_constructor_valid_load_dimensions():
    """
    Tests that the ``ProblemData`` constructor does not raise a ``ValueError``
    when client dimensions for pickup and delivery match the vehicle type
    dimensions for capacity.
    """
    data = ProblemData(
        clients=[
            Client(x=0, y=0, delivery=[1, 2], pickup=[1, 2]),
            Client(x=1, y=1, delivery=[1, 2], pickup=[1, 2]),
        ],
        depots=[Depot(x=0, y=0)],
        vehicle_types=[
            VehicleType(2, capacity=[1, 2]),
            VehicleType(2, capacity=[1, 2]),
        ],
        distance_matrices=[np.zeros((3, 3), dtype=int)],
        duration_matrices=[np.zeros((3, 3), dtype=int)],
    )
    assert_equal(data.num_load_dimensions, 2)


@pytest.mark.parametrize(
    ("start_depot", "end_depot"),
    [(0, 1), (1, 0), (1, 1)],
)
def test_raises_if_vehicle_and_depot_time_windows_do_not_overlap(
    start_depot: int,
    end_depot: int,
):
    """
    Tests that the ProblemData constructor raises when a vehicle type's time
    window (shift) does not at least overlap with that of the vehicle type's
    start and end depots.
    """
    depot1 = Depot(x=0, y=0, tw_early=0, tw_late=10)
    depot2 = Depot(x=0, y=0, tw_early=15, tw_late=25)
    vehicle_type = VehicleType(  # overlap with first depot, but not second
        tw_early=5,
        tw_late=10,
        start_depot=start_depot,
        end_depot=end_depot,
    )

    with assert_raises(ValueError):
        ProblemData(
            clients=[],
            depots=[depot1, depot2],
            vehicle_types=[vehicle_type],
            distance_matrices=[np.zeros((2, 2), dtype=int)],
            duration_matrices=[np.zeros((2, 2), dtype=int)],
        )

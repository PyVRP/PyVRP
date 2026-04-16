import pickle

import numpy as np
import pytest
from numpy.random import default_rng
from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import (
    Client,
    ClientGroup,
    Depot,
    Location,
    ProblemData,
    VehicleType,
)


def test_problem_data_raises_when_no_depot_is_provided():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    no depots are provided.
    """
    with assert_raises(ValueError):
        ProblemData(
            locations=[],
            clients=[],
            depots=[],
            vehicle_types=[VehicleType()],
            distance_matrices=[np.asarray([[]], dtype=int)],
            duration_matrices=[np.asarray([[]], dtype=int)],
        )

    # One (or more) depots should not raise.
    ProblemData(
        locations=[Location(0, 0)],
        clients=[],
        depots=[Depot(0)],
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
            locations=[Location(0, 0)],
            clients=[],
            depots=[Depot(0)],
            vehicle_types=[],
            distance_matrices=[np.asarray([[0]])],
            duration_matrices=[np.asarray([[0]])],
        )

    # One (or more) vehicle types should not raise.
    ProblemData(
        locations=[Location(0, 0)],
        clients=[],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.asarray([[0]])],
        duration_matrices=[np.asarray([[0]])],
    )


@pytest.mark.parametrize(
    "matrix",
    [
        np.asarray([[0, 0]]),  # num rows < num locations
        np.asarray([[], []]),  # num cols < num locations
        np.asarray([[0, 0], [0, 0], [0, 0]]),  # num rows > num locations
        np.asarray([[0, 0, 0], [0, 0, 0]]),  # num cols > num locations
    ],
)
def test_problem_data_raises_when_incorrect_matrix_dimensions(matrix):
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    the distance or duration matrix does not match the number of locations.
    """
    locations = [Location(0, 0), Location(0, 0)]
    clients = [Client(1)]
    depots = [Depot(0)]
    vehicle_types = [VehicleType()]
    other_matrix = np.zeros((2, 2), dtype=int)  # this one's OK

    with assert_raises(ValueError):
        ProblemData(
            locations,
            clients,
            depots,
            vehicle_types,
            [matrix],
            [other_matrix],
        )

    with assert_raises(ValueError):
        ProblemData(
            locations,
            clients,
            depots,
            vehicle_types,
            [other_matrix],
            [matrix],
        )


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
    locations = [Location(0, 0), Location(0, 0)]
    clients = [Client(1)]
    depots = [Depot(0)]
    vehicle_types = [VehicleType()]

    with assert_raises(ValueError):
        ProblemData(
            locations,
            clients,
            depots,
            vehicle_types,
            [dist_mat],
            [dur_mat],
        )


def test_problem_data_replace_no_changes():
    """
    Tests that when using ``ProblemData.replace()`` without any arguments
    returns a new instance with different objects, but with the same values.
    """
    locs = [Location(0, 0), Location(0, 0)]
    clients = [Client(1)]
    depots = [Depot(0)]
    vehicle_types = [VehicleType()]
    mat = np.zeros((2, 2), dtype=int)
    original = ProblemData(locs, clients, depots, vehicle_types, [mat], [mat])

    new = original.replace()

    assert_(new is not original)

    for idx in range(new.num_clients):
        assert_(new.client(idx) is not original.client(idx))
        assert_equal(new.client(idx).location, original.client(idx).location)

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
    mat = np.zeros((2, 2), dtype=int)
    original = ProblemData(
        locations=[Location(0, 0), Location(0, 0)],
        clients=[Client(1, delivery=[0])],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(2, capacity=[1])],
        distance_matrices=[mat],
        duration_matrices=[mat],
    )

    # Let's replace the clients, vehicle types, and the distance matrix, each
    # with different values than in the original data. The duration matrix
    # is left unchanged.
    new = original.replace(
        clients=[Client(1, delivery=[0])],
        vehicle_types=[VehicleType(3, [4]), VehicleType(5, [6])],
        distance_matrices=[np.where(np.eye(2), 0, 2)],
    )

    assert_(new is not original)
    assert_(new.client(0) is not original.client(0))

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
    mismatched shape between the locations and the distance/duration matrices.
    """
    mat = np.zeros((2, 2), dtype=int)
    data = ProblemData(
        locations=[Location(0, 0), Location(0, 0)],
        clients=[Client(1)],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(2)],
        distance_matrices=[mat],
        duration_matrices=[mat],
    )

    with assert_raises(IndexError):  # matrices are 2x2
        data.replace(locations=[])

    with assert_raises(ValueError):  # two locations
        data.replace(distance_matrices=[np.where(np.eye(3), 0, 1)])

    with assert_raises(ValueError):  # two locations
        data.replace(duration_matrices=[np.where(np.eye(3), 0, 1)])


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
        locations=[Location(0, 0), *(Location(0, 0) for _ in range(size - 1))],
        clients=[
            Client(location=idx + 1, tw_late=10) for idx in range(size - 1)
        ],
        depots=[Depot(location=0)],
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
        locations=[Location(0, 0)],
        clients=[],
        depots=[Depot(0)],
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
        locations=[Location(0, 0), Location(0, 1)],
        clients=[Client(1)],
        depots=[Depot(0)],
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


def test_depot_client_raises_invalid_index(ok_small):
    """
    Tests that calling depot(idx) and client(idx) raises when the index is out
    of bounds.
    """
    assert_equal(ok_small.num_depots, 1)
    with assert_raises(IndexError):
        ok_small.depot(1)

    assert_equal(ok_small.num_clients, 4)
    with assert_raises(IndexError):
        ok_small.client(4)


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
            locations=[Location(1, 1)],
            clients=[],
            depots=[Depot(0)],
            vehicle_types=[VehicleType()],
            distance_matrices=[[[0]]],
            duration_matrices=[[[0]]],
            groups=[ClientGroup()],  # empty group - this should raise
        )


@pytest.mark.parametrize(
    ("groups", "index"),
    [
        ([], 0),  # index 0, but there are no groups
        ([ClientGroup([0])], 1),  # there is one group, but index is 1
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
            locations=[Location(1, 1), Location(1, 1)],
            clients=[Client(1, required=False, group=index)],
            depots=[Depot(0)],
            vehicle_types=[VehicleType()],
            distance_matrices=[np.zeros((2, 2))],
            duration_matrices=[np.zeros((2, 2))],
            groups=groups,
        )


def test_raises_invalid_group_client_indices():
    """
    Tests that group client indices that are outside the range of clients
    results in an IndexError.
    """
    with assert_raises(IndexError):
        ProblemData(
            locations=[Location(1, 1), Location(1, 1)],
            clients=[Client(1, required=False, group=0)],
            depots=[Depot(0)],
            vehicle_types=[VehicleType()],
            distance_matrices=[np.zeros((2, 2))],
            duration_matrices=[np.zeros((2, 2))],
            groups=[ClientGroup([0, 1])],
        )


def test_raises_wrong_mutual_group_referencing():
    """
    Groups should reference the clients in the group, and vice versa, the
    client should reference the group. If this is not done correctly, a
    ValueError should be thrown.
    """
    with assert_raises(ValueError):
        ProblemData(
            locations=[Location(1, 1), Location(1, 1), Location(2, 2)],
            # The client references the first group, which does not contain the
            # client. That should raise.
            clients=[
                Client(1, required=False, group=0),
                Client(2, required=False, group=0),
            ],
            depots=[Depot(0)],
            vehicle_types=[VehicleType()],
            distance_matrices=[np.zeros((3, 3))],
            duration_matrices=[np.zeros((3, 3))],
            groups=[ClientGroup([2])],
        )

    with assert_raises(ValueError):
        ProblemData(
            locations=[Location(1, 1), Location(1, 1), Location(2, 2)],
            clients=[Client(1), Client(2)],
            depots=[Depot(0)],
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
            locations=[Location(1, 1), Location(1, 1)],
            clients=[Client(1, required=True, group=0)],
            depots=[Depot(0)],
            vehicle_types=[VehicleType()],
            distance_matrices=[np.zeros((2, 2))],
            duration_matrices=[np.zeros((2, 2))],
            groups=[ClientGroup([0])],
        )


def test_replacing_client_groups(ok_small):
    """
    Tests that replacing mutually exclusive client groups works well.
    """
    assert_equal(ok_small.num_groups, 0)
    assert_equal(ok_small.groups(), [])

    # Let's add the first client to a group, and define a new data instance
    # that has a mutually exclusive group.
    clients = ok_small.clients()
    clients[0] = Client(1, delivery=[1], required=False, group=0)
    data = ok_small.replace(clients=clients, groups=[ClientGroup([0])])

    # There should now be a single client group (at index 0) that has the first
    # client as its only member.
    assert_equal(data.num_groups, 1)
    assert_equal(data.group(0).clients, [0])


def test_pickle_data(ok_small, rc208):
    """
    Tests that problem data instances can be serialised and unserialised.
    """
    bytes = pickle.dumps(ok_small)
    assert_equal(pickle.loads(bytes), ok_small)

    bytes = pickle.dumps(rc208)
    assert_equal(pickle.loads(bytes), rc208)


def test_problem_data_raises_when_pickup_and_delivery_dimensions_differ():
    """
    Tests that the ``ProblemData`` constructor raises a ``ValueError`` when
    clients are provided with different dimensions for delivery and pickup.
    """
    with assert_raises(ValueError):
        ProblemData(
            locations=[Location(0, 0), Location(0, 0), Location(1, 1)],
            clients=[
                Client(1, delivery=[1, 2], pickup=[1, 2]),
                Client(2, delivery=[1, 2, 3], pickup=[1, 2, 3]),
            ],
            depots=[Depot(0)],
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
            locations=[Location(0, 0)],
            clients=[],
            depots=[Depot(0)],
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
            locations=[Location(0, 0), Location(0, 0), Location(1, 1)],
            clients=[
                Client(1, delivery=[1, 2], pickup=[1, 2]),
                Client(2, delivery=[1, 2], pickup=[1, 2]),
            ],
            depots=[Depot(0)],
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
        locations=[Location(0, 0), Location(0, 0), Location(1, 1)],
        clients=[
            Client(1, delivery=[1, 2], pickup=[1, 2]),
            Client(2, delivery=[1, 2], pickup=[1, 2]),
        ],
        depots=[Depot(0)],
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
    depot1 = Depot(0, tw_early=0, tw_late=10)
    depot2 = Depot(1, tw_early=15, tw_late=25)
    vehicle_type = VehicleType(  # overlap with first depot, but not second
        tw_early=5,
        tw_late=10,
        start_depot=start_depot,
        end_depot=end_depot,
    )

    with assert_raises(ValueError):
        ProblemData(
            locations=[Location(0, 0), Location(0, 0)],
            clients=[],
            depots=[depot1, depot2],
            vehicle_types=[vehicle_type],
            distance_matrices=[np.zeros((2, 2), dtype=int)],
            duration_matrices=[np.zeros((2, 2), dtype=int)],
        )


def test_validate_raises_for_invalid_reload_depot(ok_small):
    """
    Tests that the ProblemData's constructor validates the reload locations
    reference existing depots, and raises if something is wrong.
    """
    assert_equal(ok_small.num_depots, 1)

    old_vehicle_type = ok_small.vehicle_type(0)
    new_vehicle_type = old_vehicle_type.replace(reload_depots=[1])
    assert_equal(new_vehicle_type.reload_depots, [1])

    # First check if the constructor raises. There's just one depot, but the
    # reload depot references a depot at index 1, which does not exist.
    mat = np.zeros((1, 1), dtype=int)
    with assert_raises(IndexError):
        ProblemData(
            locations=[Location(0, 0)],
            clients=[],
            depots=[Depot(0)],
            vehicle_types=[new_vehicle_type],
            distance_matrices=[mat],
            duration_matrices=[mat],
        )

    # Replacing the vehicle type on the OkSmall instance should similarly raise
    # during argument validation.
    with assert_raises(IndexError):
        ok_small.replace(vehicle_types=[new_vehicle_type])


def test_has_time_windows(small_cvrp, pr107, small_spd, ok_small, rc208):
    """
    Tests that ``has_time_windows()`` correctly identifies whether instances
    have meaningful time window restrictions.
    """
    assert_(ok_small.has_time_windows())  # is VRPTW
    assert_(rc208.has_time_windows())  # is also VRPTW

    assert_(not pr107.has_time_windows())  # is TSP
    assert_(not small_cvrp.has_time_windows())  # is CVRP
    assert_(not small_spd.has_time_windows())  # is VRPSPD


def test_accessors(ok_small):
    """
    Tests accessing locations, depots, and clients on the data instance.
    """
    locations = ok_small.locations()
    assert_equal(ok_small.location(0), locations[0])

    depots = ok_small.depots()
    assert_equal(ok_small.depot(0), depots[0])

    clients = ok_small.clients()
    assert_equal(ok_small.client(0), clients[0])
    assert_equal(ok_small.client(3), clients[3])


def test_raises_unknown_location():
    """
    Tests that creating depots or clients referencing unknown locations raises.
    """
    with assert_raises(IndexError):
        ProblemData(
            locations=[Location(0, 0)],
            depots=[Depot(location=1)],  # invalid location
            clients=[],
            vehicle_types=[VehicleType()],
            distance_matrices=[np.zeros((1, 1), dtype=int)],
            duration_matrices=[np.zeros((1, 1), dtype=int)],
        )

    with assert_raises(IndexError):
        ProblemData(
            locations=[Location(0, 0), Location(0, 0)],
            depots=[Depot(location=0)],  # valid location
            clients=[Client(location=2)],  # invalid location
            vehicle_types=[VehicleType()],
            distance_matrices=[np.zeros((2, 2), dtype=int)],
            duration_matrices=[np.zeros((2, 2), dtype=int)],
        )

    # Both location references are valid, so this should pass.
    ProblemData(
        locations=[Location(0, 0), Location(0, 0)],
        depots=[Depot(location=0)],  # valid location
        clients=[Client(location=1)],  # valid location
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((2, 2), dtype=int)],
        duration_matrices=[np.zeros((2, 2), dtype=int)],
    )

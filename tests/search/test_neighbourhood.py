import numpy as np
from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import Depot, Location, ProblemData, VehicleType
from pyvrp.search import NeighbourhoodParams, compute_neighbours


def test_neighbourhood_params_raises_for_empty_neighbourhoods():
    """
    Test that ``NeighbourhoodParams`` raises for empty neighbourhoods.
    """
    with assert_raises(ValueError):
        NeighbourhoodParams(num_neighbours=0)


# fmt: off
@mark.parametrize(
    (
        "weight_wait_time",
        "num_neighbours",
        "symmetric_proximity",
        "idx_check",
        "expected_neighbours_check",
    ),
    [
        (20, 10, True, 1,
         {0, 2, 3, 4, 5, 6, 7, 44, 45, 99}),
        # From original C++ implementation
        (18, 34, True, 0,
         {1, 2, 3, 4, 5, 6, 7, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 52,
          53, 54, 59, 60, 67, 68, 69, 72, 77, 78, 80, 87, 89, 97, 99}),
        (18, 34, True, 98,
         {8, 9, 10, 11, 12, 13, 14, 15, 19, 21, 23, 46, 51, 52, 54, 55, 56, 57,
          58, 59, 63, 64, 65, 68, 73, 79, 81, 82, 85, 86, 87, 89, 90, 97}),
    ],
)
# fmt: on
def test_compute_neighbours(
    rc208,
    weight_wait_time: int,
    num_neighbours: int,
    symmetric_proximity: bool,
    idx_check: int,
    expected_neighbours_check: set[int],
):
    """
    Tests ``compute_neighbours`` on several well-understood cases.
    """
    params = NeighbourhoodParams(
        weight_wait_time,
        num_neighbours,
        symmetric_proximity,
    )
    neighbours = compute_neighbours(rc208, params)

    assert_equal(len(neighbours), rc208.num_clients)

    # We compare sets because the expected data (from the old C++
    # implementation) sorts by client ID (ascending), not proximity.
    assert_equal(set(neighbours[idx_check]), expected_neighbours_check)

    for neighb in neighbours:
        assert_equal(len(neighb), num_neighbours)


def test_neighbours_are_sorted_by_proximity(small_cvrp):
    """
    Tests that the neighbourhood lists sort by their proximity: closest first.
    """
    params = NeighbourhoodParams(0, small_cvrp.num_clients)
    neighbours = compute_neighbours(small_cvrp, params)
    clients = range(small_cvrp.num_clients)

    # Only consider client location distances; depots do not factor into the
    # neighbourhoods.
    distances = small_cvrp.distance_matrix(profile=0)
    distances = distances[small_cvrp.num_depots:, small_cvrp.num_depots:]

    for client in clients:
        # Proximity is completely based on distance. We break ties by index
        # (using stable sort). The test below checks that this is the same as
        # what comes out of the granular neighbourhood calculation.
        valid = np.array([other for other in clients if other != client])
        dists = distances[client, valid]
        by_proximity = valid[np.argsort(dists, kind="stable")]
        assert_equal(by_proximity, neighbours[client])


def test_more_neighbours_than_instance_size(rc208):
    """
    Tests that a value for ``num_neighbours`` larger than the number of clients
    in the instance results in neighbourhoods of maximum size.
    """
    params = NeighbourhoodParams(num_neighbours=rc208.num_clients)
    neighbours = compute_neighbours(rc208, params)

    for neighb in neighbours[1:]:
        assert_equal(len(neighb), rc208.num_clients - 1)


def test_proximity_with_prizes(prize_collecting):
    """
    Tests that prizes factor into the neighbourhood structure, and offset
    travel costs somewhat.
    """
    params = NeighbourhoodParams(0, num_neighbours=10)
    neighbours = compute_neighbours(prize_collecting, params)

    # We compare the number of times clients C19 and C35 are in other clients'
    # neighbourhoods. Client 19 is at location (57, 58), client 35 at (63, 69).
    # They're fairly close to each other, in one corner of the plane. The
    # biggest difference is in prizes: client 19 has a prize of 33, whereas
    # client 35 only yields 8. As a consequence, 35 should be in many fewer
    # neighbourhoods than 19.
    count_19 = sum(19 in n for n in neighbours)
    count_35 = sum(35 in n for n in neighbours)
    assert_(count_19 > count_35)


def test_proximity_with_mutually_exclusive_groups(
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that clients that are a member of a mutually exclusive client group
    are not in each other's neighbourhood.
    """
    params = NeighbourhoodParams(0, num_neighbours=1)
    neighbours = compute_neighbours(ok_small_mutually_exclusive_groups, params)

    group = ok_small_mutually_exclusive_groups.group(0)
    members = group.clients
    for client in members:
        assert_(all(other not in neighbours[client] for other in members))


def test_different_routing_costs(ok_small):
    """
    High-level smoke test that checks that the granular neighbourhood takes
    into account the unit distance and duration costs of different vehicle
    types.
    """
    rng = np.random.default_rng(seed=42)
    new_dur = rng.integers(0, 1_000, size=(5, 5))
    np.fill_diagonal(new_dur, 0)

    # Because the OkSmall instance has the same distance and duration matrices,
    # it is hard to make this test work. So we first introduce a new duration
    # matrix.
    new_data = ok_small.replace(duration_matrices=[new_dur])
    new_neighbours = compute_neighbours(new_data)

    # Now we also add vehicle types with different cost profiles: the first
    # vehicle type is like the original and cares only about distance, whereas
    # the second type aims to minimise route duration. This should result in a
    # different neighbourhood structure.
    orig_type = ok_small.vehicle_type(0)
    different_cost_data = new_data.replace(
        vehicle_types=[
            orig_type.replace(unit_distance_cost=1, unit_duration_cost=0),
            orig_type.replace(unit_distance_cost=0, unit_duration_cost=1),
        ],
    )
    different_cost_neighbours = compute_neighbours(different_cost_data)
    assert_(different_cost_neighbours != new_neighbours)


def test_multiple_routing_profiles(ok_small):
    """
    Tests the granular neighbourhood selects the right profiles in the
    proximity calculation when working with multiple routing profiles.
    """
    huge_mat = np.where(np.eye(ok_small.num_locations), 0, 10_000)
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(1, capacity=[10], profile=1),
            *ok_small.vehicle_types(),
        ],
        distance_matrices=[huge_mat, *ok_small.distance_matrices()],
        duration_matrices=[huge_mat, *ok_small.duration_matrices()],
    )

    # The new routing profile has huge distances and durations, but the
    # original profile and vehicles are still available. The neighbourhood
    # computation should recognise this and use the best profile for
    # neighbourhood computations, resulting in the same neighbourhood as with
    # the original (unchanged) data.
    assert_equal(compute_neighbours(data), compute_neighbours(ok_small))


def test_zero_clients():
    """
    Tests that the neighbourhood for an instance with zero clients is empty.
    """
    data = ProblemData(
        locations=[Location(0, 0)],
        clients=[],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((1, 1), dtype=int)],
        duration_matrices=[np.zeros((1, 1), dtype=int)],
    )

    assert_equal(compute_neighbours(data), [])

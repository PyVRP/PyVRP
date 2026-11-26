import numpy as np
from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp import (
    Activity,
    Client,
    Depot,
    Location,
    ProblemData,
    Shipment,
    VehicleType,
)
from pyvrp.search import NeighbourhoodParams, compute_neighbours


def test_neighbourhood_params_raises_for_empty_neighbourhoods():
    """
    Test that ``NeighbourhoodParams`` raises for empty neighbourhoods.
    """
    with assert_raises(ValueError):
        NeighbourhoodParams(num_neighbours=0)


def test_predecessor_proximity():
    """
    Tests that U's neighbours rank predecessors V using h(V, U) - rho(V).
    """
    distances = np.array(
        [[0, 100, 100, 100], [100, 0, 1, 4], [100, 8, 0, 5], [100, 1, 9, 0]]
    )
    data = ProblemData(
        locations=[Location(idx, 0) for idx in range(4)],
        clients=[Client(1), Client(2), Client(3)],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[distances],
        duration_matrices=[np.zeros_like(distances)],
    )

    # Incoming scores for C2 are 1.5 and -1.5, favouring C1 despite C0
    # being closer. Outgoing scoring would favour C0.
    params = NeighbourhoodParams(num_neighbours=1)
    neighbours = compute_neighbours(data, params)
    assert_equal(neighbours[Activity("C2")], [Activity("C1")])


@mark.parametrize(
    ("weight", "service_duration", "expected"),
    [(0.2, 0, "C0"), (0.3, 0, "C0"), (1, 0, "C1"), (1, 40, "C0")],
)
def test_waiting_in_arc_cost_and_opportunity_cost(
    weight, service_duration, expected
):
    """
    Tests waiting in both terms, including service time and time feasibility.
    """
    distances = np.full((4, 4), 100, dtype=int)
    np.fill_diagonal(distances, 0)
    distances[1, 2], distances[1, 3], distances[2, 3] = 20, 10, 12
    data = ProblemData(
        locations=[Location(idx, 0) for idx in range(4)],
        clients=[
            Client(1, tw_late=0, service_duration=service_duration),
            Client(2, tw_early=20, tw_late=20),
            Client(3, tw_early=40, tw_late=40),
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[distances],
        duration_matrices=[np.zeros_like(distances)],
    )

    # Without service, scores are -5 + 10 * weight and 0. Weight 0.3 also
    # catches dividing by ten instead of the feasible alternative count.
    # Service 40 excludes C0 -> C1, leaving a tie resolved by index.
    params = NeighbourhoodParams(weight_wait_time=weight, num_neighbours=1)
    neighbours = compute_neighbours(data, params)
    assert_equal(neighbours[Activity("C2")], [Activity(expected)])


def test_opportunity_cost_uses_ten_cheapest_alternatives():
    """
    Tests that an expensive eleventh alternative does not increase rho.
    """
    distances = np.ones((13, 13), dtype=int)
    distances[:, 1] = 1_000  # most predecessors are far from U = C0
    distances[2, 1] = 10  # V = C1
    distances[3, 1] = 8  # W = C2
    distances[2, 12] = 1_000  # expensive eleventh alternative for V
    np.fill_diagonal(distances, 0)
    data = ProblemData(
        locations=[Location(idx, 0) for idx in range(13)],
        clients=[Client(idx) for idx in range(1, 13)],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[distances],
        duration_matrices=[np.zeros_like(distances)],
    )

    # Scores are 8.1 and 7; including the eleventh alternative favours C1.
    neighbours = compute_neighbours(
        data, NeighbourhoodParams(num_neighbours=1)
    )
    assert_equal(neighbours[Activity("C0")], [Activity("C2")])


def test_arc_cost_uses_a_complete_feasible_profile():
    """
    Tests that cheap distance and feasible duration come from the same profile.
    """
    distances = np.ones((4, 4), dtype=int)
    np.fill_diagonal(distances, 0)
    distances[1, 3], distances[2, 3] = 1, 5
    other_distances = distances.copy()
    other_distances[1, 3] = 20
    durations = np.zeros_like(distances)
    durations[1, 3] = 100  # the cheap C0 -> C2 arc is infeasible
    data = ProblemData(
        locations=[Location(idx, 0) for idx in range(4)],
        clients=[
            Client(1, tw_late=0),
            Client(2, tw_late=0),
            Client(3, tw_early=10, tw_late=10),
        ],
        depots=[Depot(0)],
        vehicle_types=[VehicleType(profile=0), VehicleType(profile=1)],
        distance_matrices=[distances, other_distances],
        duration_matrices=[durations, np.zeros_like(durations)],
    )

    # Scores into C2 are 10.5 and 3. Mixing profiles would favour C0.
    neighbours = compute_neighbours(
        data, NeighbourhoodParams(num_neighbours=1)
    )
    assert_equal(neighbours[Activity("C2")], [Activity("C1")])


def test_more_neighbours_than_instance_size(rc208):
    """
    Tests that a value for ``num_neighbours`` larger than the number of clients
    in the instance results in neighbourhoods of maximum size.
    """
    params = NeighbourhoodParams(num_neighbours=rc208.num_clients)
    neighbours = compute_neighbours(rc208, params)

    for neighb in neighbours.values():
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
    count_19 = sum(Activity("C19") in n for n in neighbours.values())
    count_35 = sum(Activity("C35") in n for n in neighbours.values())
    assert_(count_19 > count_35)


def test_proximity_with_mutually_exclusive_groups(
    ok_small_mutually_exclusive_groups,
):
    """
    Tests that clients that are a member of a mutually exclusive group are not
    in each other's neighbourhood.
    """
    params = NeighbourhoodParams(0, num_neighbours=1)
    neighbours = compute_neighbours(ok_small_mutually_exclusive_groups, params)

    group = ok_small_mutually_exclusive_groups.group(0)
    members = group.clients
    for client in members:
        act = Activity(f"C{client}")
        assert_(
            all(
                Activity(f"C{other}") not in neighbours[act]
                for other in members
            )
        )


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


def test_zero_clients_or_shipments():
    """
    Tests that the neighbourhood for an instance with zero clients and
    shipments is empty.
    """
    data = ProblemData(
        locations=[Location(0, 0)],
        clients=[],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[np.zeros((1, 1), dtype=int)],
        duration_matrices=[np.zeros((1, 1), dtype=int)],
    )

    assert_equal(compute_neighbours(data), {})


def test_shipments_exclude_either_activity_in_neighbourhood(small_shipments):
    """
    Tests that shipments exclude their own activities from the neighbourhood.
    """
    neighbours = compute_neighbours(small_shipments)

    for shipment in range(small_shipments.num_shipments):
        delivery = Activity(f"U{shipment}")
        assert_(delivery not in neighbours)  # only includes pickup activities

        pickup = Activity(f"L{shipment}")
        neighbourhood = neighbours[pickup]
        assert_(pickup not in neighbourhood)  # cannot be in own neighbourhood
        assert_(delivery not in neighbourhood)  # nor can delivery

        # The neighbourhood contains pickup and delivery activities for each
        # shipment, but excludes its own.
        assert_equal(len(neighbourhood), 2 * small_shipments.num_shipments - 2)


def test_shipment_proximity_uses_pickup_head():
    """
    Tests that shipment proximity ranks predecessors of the pickup.
    """
    distances = np.full((5, 5), 100, dtype=int)
    np.fill_diagonal(distances, 0)
    distances[3, 1] = distances[1, 3] = 10  # pickup closest to C0
    distances[4, 2] = distances[2, 4] = 1  # delivery closest to C1

    data = ProblemData(
        locations=[Location(idx, 0) for idx in range(5)],
        clients=[Client(1), Client(2)],
        depots=[Depot(0)],
        vehicle_types=[VehicleType()],
        distance_matrices=[distances],
        duration_matrices=[distances],
        shipments=[Shipment(3, 4)],
    )

    # Pickup scores are -60 and 33. The nearby delivery must not favour C1.
    params = NeighbourhoodParams(0, num_neighbours=1)
    neighbours = compute_neighbours(data, params)

    assert_equal(neighbours[Activity("L0")], [Activity("C0")])


def test_mixed_client_shipments(small_shipments):
    """
    Tests the neighbourhood of a small instance with mixed clients and
    shipments.
    """
    clients = [Client(2, delivery=[0]), Client(5, delivery=[1])]
    data = small_shipments.replace(clients=clients)

    neighbours = compute_neighbours(data)
    assert_equal(len(neighbours), 6)  # for each client and shipment pickup

    # C0 and L0 share a location and should be each other's first neighbour.
    assert_equal(neighbours[Activity("C0")][0], Activity("L0"))
    assert_equal(neighbours[Activity("L0")][0], Activity("C0"))

    # A client can neighbour the other client and all eight shipment steps.
    for idx in range(data.num_clients):
        client = Activity(f"C{idx}")
        assert_equal(len(neighbours[client]), 9)
        assert_(client not in neighbours[client])

    # A pickup can neighbour both clients and the other three shipments.
    for idx in range(data.num_shipments):
        pickup = Activity(f"L{idx}")
        delivery = Activity(f"U{idx}")
        assert_equal(len(neighbours[pickup]), 8)
        assert_(pickup not in neighbours[pickup])
        assert_(delivery not in neighbours[pickup])

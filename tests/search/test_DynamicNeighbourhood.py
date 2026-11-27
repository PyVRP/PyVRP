import numpy as np
import pytest

from pyvrp import (
    Activity,
    Client,
    Depot,
    Location,
    ProblemData,
    Shipment,
    VehicleType,
)
from pyvrp.search import PerturbationManager, compute_neighbours
from pyvrp.search._search import LocalSearch


def make_search(num_clients=85, shipment=False):
    num_locations = 1 + num_clients + 2 * shipment
    matrix = np.ones((num_locations, num_locations), dtype=np.int64)
    np.fill_diagonal(matrix, 0)
    data = ProblemData(
        locations=[Location(idx, 0) for idx in range(num_locations)],
        clients=[Client(idx + 1) for idx in range(num_clients)],
        depots=[Depot(0)],
        shipments=[Shipment(num_clients + 1, num_clients + 2)]
        if shipment
        else [],
        vehicle_types=[VehicleType(3)],
        distance_matrices=[matrix],
        duration_matrices=[matrix],
    )
    backbone = compute_neighbours(data)
    native = LocalSearch(data, backbone, PerturbationManager(data))
    return native, backbone


@pytest.fixture
def ring():
    native, backbone = make_search()
    # Reverse ranks are at least 24: reciprocal activation cannot obscure
    # the 15/30/60 boundaries when only one row widens.
    candidates = {
        Activity(f"C{idx}"): [
            Activity(f"C{(idx + step) % 85}") for step in range(1, 61)
        ]
        for idx in range(85)
    }
    return native, candidates, backbone


@pytest.fixture
def shipment_ring():
    native, backbone = make_search(84, shipment=True)
    owners = [Activity("L0")] + [Activity(f"C{idx}") for idx in range(84)]
    candidates = {}
    for idx, owner in enumerate(owners):
        row = [owners[(idx + step) % 85] for step in range(1, 61)]
        candidates[owner] = [
            Activity("U0") if act == owners[0] else act for act in row
        ]
    return native, candidates, backbone


def test_default_patience_counts_each_owner_once_per_iteration(ring):
    native, candidates, backbone = ring
    dynamic = native.configure_dynamic_neighbourhood(candidates)
    client = Activity("C0")
    assert dynamic.neighbours[client] == candidates[client][:15]

    for iteration in range(1, 2001):
        dynamic.touch(client)
        dynamic.touch(client)
        dynamic.update(False)
        width = 15 if iteration < 1000 else 30 if iteration < 2000 else 60
        assert dynamic.neighbours[client] == candidates[client][:width]

    assert native.active_neighbours == dynamic.neighbours
    assert native.neighbours == backbone


def test_unvisited_rows_do_not_age_and_only_cached_rows_reset(ring):
    native, candidates, _ = ring
    dynamic = native.configure_dynamic_neighbourhood(candidates, patience=2)
    client, other = Activity("C0"), Activity("C1")
    dynamic.touch(client)
    dynamic.update(False)
    for _ in range(10):
        dynamic.update(False)
    assert dynamic.neighbours[client] == candidates[client][:15]

    dynamic.touch(other)
    dynamic.update(True)
    dynamic.touch(client)
    dynamic.update(False)
    assert dynamic.neighbours[client] == candidates[client][:30]

    dynamic.touch(client)
    dynamic.update(False)
    dynamic.touch(other)
    dynamic.update(True)
    dynamic.touch(client)
    dynamic.update(False)
    assert dynamic.neighbours[client] == candidates[client]

    dynamic.touch(client)
    dynamic.update(False)  # Leave a nonzero failure count at full size.
    dynamic.touch(client)
    dynamic.update(True)
    assert dynamic.neighbours[client] == candidates[client][:15]
    dynamic.touch(client)
    dynamic.update(False)
    assert dynamic.neighbours[client] == candidates[client][:15]


def test_pickup_and_delivery_share_participations_and_resets(shipment_ring):
    native, candidates, _ = shipment_ring
    dynamic = native.configure_dynamic_neighbourhood(candidates, patience=3)
    pickup, delivery = Activity("L0"), Activity("U0")
    for iteration in range(1, 7):
        dynamic.touch(pickup)
        dynamic.touch(delivery)
        dynamic.update(False)
        width = 15 if iteration < 3 else 30 if iteration < 6 else 60
        assert dynamic.neighbours[pickup] == candidates[pickup][:width]

    dynamic.touch(delivery)
    dynamic.update(True)
    assert dynamic.neighbours[pickup] == candidates[pickup][:15]


@pytest.mark.parametrize("refresh", [False, True])
def test_only_the_50_most_recent_owners_age(refresh):
    native, _ = make_search()
    candidates = {Activity(f"C{idx}"): [] for idx in range(85)}
    row = [Activity(f"C{idx}") for idx in range(51, 85)]
    candidates[Activity("C0")] = row
    candidates[Activity("C1")] = row
    dynamic = native.configure_dynamic_neighbourhood(candidates, patience=1)
    for idx in range(50):
        dynamic.touch(Activity(f"C{idx}"))
    if refresh:
        dynamic.touch(Activity("C0"))
    dynamic.touch(Activity("C50"))
    dynamic.update(False)

    assert dynamic.neighbours[Activity("C0")] == row[: 17 if refresh else 9]
    assert dynamic.neighbours[Activity("C1")] == row[: 9 if refresh else 17]


def test_reciprocal_activation_is_clipped_and_preserves_candidate_order():
    native, _ = make_search(8)
    candidates = {Activity(f"C{idx}"): [] for idx in range(8)}
    candidates[Activity("C0")] = [Activity(f"C{idx}") for idx in (1, 2, 3, 4)]
    candidates[Activity("C3")] = [Activity(f"C{idx}") for idx in (5, 6, 7, 0)]
    candidates[Activity("C4")] = [Activity(f"C{idx}") for idx in (0, 5, 6, 7)]
    candidates[Activity("C6")] = [Activity("C0")]
    dynamic = native.configure_dynamic_neighbourhood(candidates, patience=1)
    assert dynamic.neighbours[Activity("C0")] == [
        Activity("C1"),
        Activity("C4"),
    ]
    for _ in range(2):
        dynamic.touch(Activity("C3"))
        dynamic.update(False)
    assert dynamic.neighbours[Activity("C0")] == [
        Activity(f"C{idx}") for idx in (1, 3, 4)
    ]
    assert Activity("C6") not in dynamic.neighbours[Activity("C0")]


def test_replacing_backbone_detaches_the_controller(ring):
    native, candidates, backbone = ring
    dynamic = native.configure_dynamic_neighbourhood(candidates, patience=1)
    replacement = {activity: row[:3] for activity, row in backbone.items()}
    native.neighbours = replacement
    dynamic.touch(Activity("C0"))
    dynamic.update(False)
    assert native.neighbours == replacement
    assert native.active_neighbours == replacement


def test_zero_patience_is_rejected(ring):
    native, candidates, _ = ring
    with pytest.raises(ValueError):
        native.configure_dynamic_neighbourhood(candidates, patience=0)


@pytest.mark.parametrize(
    ("owner", "neighbour"),
    [
        ("C0", "D0"),
        ("C0", "C84"),
        ("C0", "L1"),
        ("C0", "U1"),
        ("C0", "C0"),
        ("L0", "L0"),
        ("L0", "U0"),
    ],
)
def test_invalid_candidates_are_rejected(shipment_ring, owner, neighbour):
    native, candidates, _ = shipment_ring
    candidates[Activity(owner)] = [Activity(neighbour)]
    with pytest.raises(ValueError):
        native.configure_dynamic_neighbourhood(candidates)


@pytest.mark.parametrize("key", [None, "D0", "U0", "C84"])
def test_missing_or_extra_owner_keys_are_rejected(shipment_ring, key):
    native, candidates, _ = shipment_ring
    if key is None:
        candidates.pop(Activity("C0"))
    else:
        candidates[Activity(key)] = []
    with pytest.raises(ValueError):
        native.configure_dynamic_neighbourhood(candidates)


@pytest.mark.parametrize(
    ("count", "quarter", "half"),
    [
        (0, 0, 0),
        (1, 1, 1),
        (2, 1, 1),
        (3, 1, 2),
        (5, 1, 3),
        (6, 2, 3),
        (7, 2, 4),
        (80, 20, 40),
    ],
)
def test_empty_short_and_large_rows(count, quarter, half):
    native, _ = make_search()
    candidates = {Activity(f"C{idx}"): [] for idx in range(85)}
    client = Activity("C0")
    candidates[client] = [Activity(f"C{idx}") for idx in range(1, count + 1)]
    dynamic = native.configure_dynamic_neighbourhood(candidates, patience=1)
    assert dynamic.neighbours[client] == candidates[client][:quarter]
    dynamic.touch(client)
    dynamic.update(False)
    assert dynamic.neighbours[client] == candidates[client][:half]
    dynamic.touch(client)
    dynamic.update(False)
    assert dynamic.neighbours[client] == candidates[client]

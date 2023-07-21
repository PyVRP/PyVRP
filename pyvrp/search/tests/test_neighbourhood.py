from typing import Set

import numpy as np
from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.search import NeighbourhoodParams, compute_neighbours
from pyvrp.tests.helpers import read


@mark.parametrize(
    (
        "weight_wait_time",
        "weight_time_warp",
        "nb_granular",
        "symmetric_proximity",
        "symmetric_neighbours",
    ),
    [
        # empty neighbourhood structure (nb_granular == 0)
        (20, 20, 0, True, False),
    ],
)
def test_neighbourhood_params_raises_for_invalid_arguments(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    symmetric_proximity: bool,
    symmetric_neighbours: bool,
):
    with assert_raises(ValueError):
        NeighbourhoodParams(
            weight_wait_time,
            weight_time_warp,
            nb_granular,
            symmetric_proximity,
            symmetric_neighbours,
        )


@mark.parametrize(
    (
        "weight_wait_time",
        "weight_time_warp",
        "nb_granular",
        "symmetric_proximity",
        "symmetric_neighbours",
    ),
    [
        # non-empty neighbourhood structure (nb_granular > 0)
        (20, 20, 1, True, False),
        # no weights for wait time or time warp should be OK
        (0, 0, 1, True, False),
    ],
)
def test_neighbourhood_params_does_not_raise_for_valid_arguments(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    symmetric_proximity: bool,
    symmetric_neighbours: bool,
):
    NeighbourhoodParams(
        weight_wait_time,
        weight_time_warp,
        nb_granular,
        symmetric_proximity,
        symmetric_neighbours,
    )


@mark.parametrize(
    (
        "weight_wait_time",
        "weight_time_warp",
        "nb_granular",
        "symmetric_proximity",
        "symmetric_neighbours",
        "idx_check",
        "expected_neighbours_check",
    ),
    [
        # fmt: off
        (20, 20, 10, True, False, 2,
         {1, 3, 4, 5, 6, 7, 8, 45, 46, 100}),
        (20, 20, 10, True, True, 2,
         {1, 3, 4, 5, 6, 7, 8, 45, 46, 60, 70, 79, 100}),
        # From original c++ implementation
        (18, 20, 34, True, False, 1,
         {2, 3, 4, 5, 6, 7, 8, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 53,
          54, 55, 60, 61, 68, 69, 70, 73, 78, 79, 81, 88, 90, 98, 100}),
        (18, 20, 34, True, False, 99,
         {9, 10, 11, 12, 13, 14, 15, 16, 20, 22, 24, 47, 52, 53, 55, 56, 57,
          58, 59, 60, 64, 65, 66, 69, 74, 80, 82, 83, 86, 87, 88, 90, 91, 98}),
        # fmt: on
    ],
)
def test_compute_neighbours(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    symmetric_proximity: bool,
    symmetric_neighbours: bool,
    idx_check: int,
    expected_neighbours_check: Set[int],
):
    data = read("data/RC208.txt", "solomon", "trunc1")
    params = NeighbourhoodParams(
        weight_wait_time,
        weight_time_warp,
        nb_granular,
        symmetric_proximity,
        symmetric_neighbours,
    )
    neighbours = compute_neighbours(data, params)

    assert_equal(len(neighbours), data.num_clients + 1)
    assert_equal(len(neighbours[0]), 0)

    # We compare sets because the expected data (from the old C++
    # implementation) sorts by client ID (ascending), not proximity.
    assert_equal(set(neighbours[idx_check]), expected_neighbours_check)

    for neighb in neighbours[1:]:
        if symmetric_neighbours:
            assert_(len(neighb) >= nb_granular)
        else:
            assert_equal(len(neighb), nb_granular)


def test_neighbours_are_sorted_by_proximity():
    # E-n22-k4 is a CVRP instance, so the time component is irrelevant.
    data = read("data/E-n22-k4.txt", round_func="trunc1")
    params = NeighbourhoodParams(0, 0, data.num_clients)
    neighbours = compute_neighbours(data, params)
    clients = list(range(1, data.num_clients + 1))

    for client in clients:
        # Proximity is completely based on distance. We break ties by index
        # (using stable sort). The test below checks that this is the same as
        # what comes out of the granular neighbourhood calculation.
        valid = np.array([other for other in clients if other != client])
        dists = [data.dist(client, other) for other in valid]
        by_proximity = valid[np.argsort(dists, kind="stable")]
        assert_equal(by_proximity, neighbours[client])


def test_symmetric_neighbours():
    data = read("data/RC208.txt", "solomon", "trunc1")

    # Symmetric neighbourhood structure: if (i, j) is in, then so is (j, i).
    params = NeighbourhoodParams(symmetric_neighbours=True)
    sym_neighbours = [set(n) for n in compute_neighbours(data, params)]

    for client in range(data.num_clients + 1):
        for neighbour in sym_neighbours[client]:
            assert_(client in sym_neighbours[neighbour])

    # But when neighbours are not symmetrised, this is typically not the case.
    params = NeighbourhoodParams(symmetric_neighbours=False)
    asym_neighbours = [set(n) for n in compute_neighbours(data, params)]
    assert_(sym_neighbours != asym_neighbours)


def test_more_neighbours_than_instance_size():
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    params = NeighbourhoodParams(nb_granular=data.num_clients)
    neighbours = compute_neighbours(data, params)

    for neighb in neighbours[1:]:
        assert_equal(len(neighb), data.num_clients - 1)


def test_proximity_with_prizes():
    data = read("data/p06-2-50.vrp", round_func="dimacs")
    params = NeighbourhoodParams(0, 0, nb_granular=10)
    neighbours = compute_neighbours(data, params)

    # We compare the number of times clients 20 and 36 are in other clients'
    # neighbourhoods. Client 20 is at location (57, 58), client 36 at (63, 69).
    # They're fairly close to each other, in one corner of the plane. The
    # biggest difference is in prizes: client 20 has a prize of 33, whereas
    # client 36 only yields 8. As a consequence, 36 should be in many fewer
    # neighbourhoods than 20.
    count_20 = sum(20 in n for n in neighbours)
    count_36 = sum(36 in n for n in neighbours)
    assert_(count_20 > count_36)

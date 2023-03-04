from typing import List

from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.educate import NeighbourhoodParams, compute_neighbours
from pyvrp.tests.helpers import read


@mark.parametrize(
    "weight_wait_time,"
    "weight_time_warp,"
    "nb_granular,"
    "symmetric_proximity,"
    "symmetric_neighbours",
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
    "weight_wait_time,"
    "weight_time_warp,"
    "nb_granular,"
    "symmetric_proximity,"
    "symmetric_neighbours",
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
    "weight_wait_time,"
    "weight_time_warp,"
    "nb_granular,"
    "symmetric_proximity,"
    "symmetric_neighbours,"
    "idx_check,"
    "expected_neighbours_check",
    [
        # fmt: off
        (20, 20, 10, True, False, 2,
         [1, 3, 4, 5, 6, 7, 8, 45, 46, 100]),
        (20, 20, 10, True, True, 2,
         [1, 3, 4, 5, 6, 7, 8, 45, 46, 60, 70, 79, 100]),
        # From original c++ implementation
        (18, 20, 34, True, False, 1,
         [2, 3, 4, 5, 6, 7, 8, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 53,
          54, 55, 60, 61, 68, 69, 70, 73, 78, 79, 81, 88, 90, 98, 100]),
        (18, 20, 34, True, False, 99,
         [9, 10, 11, 12, 13, 14, 15, 16, 20, 22, 24, 47, 52, 53, 55, 56, 57,
          58, 59, 60, 64, 65, 66, 69, 74, 80, 82, 83, 86, 87, 88, 90, 91, 98]),
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
    expected_neighbours_check: List[int],
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

    assert_equal(neighbours[idx_check], expected_neighbours_check)

    # Check that we have at least or exactly nb_granular depending on symmetric
    for neighb in neighbours[1:]:
        if symmetric_neighbours:
            assert_(len(neighb) >= nb_granular)
        else:
            assert_equal(len(neighb), nb_granular)


def test_more_neighbours_than_instance_size():
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    params = NeighbourhoodParams(nb_granular=data.num_clients)
    neighbours = compute_neighbours(data, params)

    for neighb in neighbours[1:]:
        assert_equal(len(neighb), data.num_clients - 1)

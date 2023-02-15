from typing import List

from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.educate import NeighbourhoodParams, compute_neighbours
from pyvrp.tests.helpers import read


@mark.parametrize(
    "weight_wait_time," "weight_time_warp," "nb_granular," "symmetric",
    [
        (20, 20, 0, False),  # empty neighbourhood structure (nbGranular)
    ],
)
def test_local_search_params_raises_for_invalid_arguments(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    symmetric: bool,
):
    with assert_raises(ValueError):
        NeighbourhoodParams(
            weight_wait_time,
            weight_time_warp,
            nb_granular,
            symmetric,
        )


@mark.parametrize(
    "weight_wait_time," "weight_time_warp," "nb_granular," "symmetric",
    [
        (20, 20, 1, False),  # non-empty neighbourhood structure (nbGranular)
        (0, 0, 1, False),  # no weights for wait time or time warp should be OK
    ],
)
def test_local_search_params_does_not_raise_for_valid_arguments(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    symmetric: bool,
):
    NeighbourhoodParams(
        weight_wait_time,
        weight_time_warp,
        nb_granular,
        symmetric,
    )


@mark.parametrize(
    "weight_wait_time,"
    "weight_time_warp,"
    "nb_granular,"
    "symmetric,"
    "idx_check,"
    "expected_neighbours_check",
    [
        (20, 20, 10, False, 2, [1, 3, 4, 5, 6, 7, 8, 45, 46, 100]),
        (20, 20, 10, True, 2, [1, 3, 4, 5, 6, 7, 8, 45, 46, 60, 70, 79, 100]),
        # From original c++ implementation
        (
            18,
            20,
            34,
            False,
            1,
            [
                2,
                3,
                4,
                5,
                6,
                7,
                8,
                36,
                37,
                38,
                39,
                40,
                41,
                42,
                43,
                44,
                45,
                46,
                53,
                54,
                55,
                60,
                61,
                68,
                69,
                70,
                73,
                78,
                79,
                81,
                88,
                90,
                98,
                100,
            ],
        ),
        (
            18,
            20,
            34,
            False,
            99,
            [
                9,
                10,
                11,
                12,
                13,
                14,
                15,
                16,
                17,
                20,
                22,
                24,
                47,
                52,
                53,
                55,
                56,
                57,
                58,
                59,
                64,
                65,
                66,
                69,
                74,
                80,
                82,
                83,
                86,
                87,
                88,
                90,
                91,
                98,
            ],
        ),
    ],
)
def test_get_neighbours(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    symmetric: bool,
    idx_check: int,
    expected_neighbours_check: List[int],
):
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    params = NeighbourhoodParams(
        weight_wait_time, weight_time_warp, nb_granular, symmetric
    )
    neighbours = compute_neighbours(data, params)

    assert_equal(len(neighbours), data.num_clients + 1)
    assert_equal(len(neighbours[0]), 0)
    assert set(neighbours[idx_check]) == set(expected_neighbours_check)
    for neighb in neighbours[1:]:
        if symmetric:
            assert_(len(neighb) >= nb_granular)
        else:
            assert_equal(len(neighb), nb_granular)


@mark.parametrize(
    "weight_wait_time,"
    "weight_time_warp,"
    "nb_granular,"
    "symmetric,"
    "idx_check,"
    "expected_neighbours_check",
    [
        (20, 20, 10, False, 2, [1, 3, 4, 5, 6, 7, 8, 45, 46, 100]),
        (20, 20, 10, True, 2, [1, 3, 4, 5, 6, 7, 8, 45, 46, 60, 70, 79, 100]),
        # From original c++ implementation
        (
            18,
            20,
            34,
            False,
            1,
            [
                2,
                3,
                4,
                5,
                6,
                7,
                8,
                36,
                37,
                38,
                39,
                40,
                41,
                42,
                43,
                44,
                45,
                46,
                53,
                54,
                55,
                60,
                61,
                68,
                69,
                70,
                73,
                78,
                79,
                81,
                88,
                90,
                98,
                100,
            ],
        ),
        (
            18,
            20,
            34,
            False,
            99,
            [
                9,
                10,
                11,
                12,
                13,
                14,
                15,
                16,
                17,
                20,
                22,
                24,
                47,
                52,
                53,
                55,
                56,
                57,
                58,
                59,
                64,
                65,
                66,
                69,
                74,
                80,
                82,
                83,
                86,
                87,
                88,
                90,
                91,
                98,
            ],
        ),
    ],
)
def test_get_neighbours_argument(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    symmetric: bool,
    idx_check: int,
    expected_neighbours_check: List[int],
):
    data = read("data/RC208.txt", "solomon", round_func="trunc")
    params = NeighbourhoodParams(
        weight_wait_time, weight_time_warp, nb_granular, symmetric
    )
    neighbours = compute_neighbours(data, params)

    from pyvrp import PenaltyManager, XorShift128
    from pyvrp.educate import LocalSearch

    seed = 42
    rng = XorShift128(seed=seed)
    pen_manager = PenaltyManager(data.vehicle_capacity)
    # neighbours = compute_neighbours(data)
    # neighbours[1] = [1, 2, 3]
    ls = LocalSearch(data, pen_manager, neighbours, rng)

    # neighbours = compute_neighbours(data)
    assert ls.neighbours == neighbours

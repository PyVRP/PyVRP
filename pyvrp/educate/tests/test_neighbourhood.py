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
    "data_loc,"
    "instance_format,"
    "round_func,"
    "weight_wait_time,"
    "weight_time_warp,"
    "nb_granular,"
    "symmetric_proximity,"
    "symmetric_neighbours,"
    "idx_check,"
    "expected_neighbours_check",
    [
        # fmt: off
        ("data/RC208.txt", "solomon", "trunc1",
         20, 20, 10, True, False,
         2, [1, 3, 4, 5, 6, 7, 8, 45, 46, 100]),
        ("data/RC208.txt", "solomon", "trunc1",
         20, 20, 10, True, True,
         2, [1, 3, 4, 5, 6, 7, 8, 45, 46, 60, 70, 79, 100]),
        # From original c++ implementation
        ("data/RC208.txt", "solomon", "trunc1",
         18, 20, 34, True, False,
         1, [2, 3, 4, 5, 6, 7, 8, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
             53, 54, 55, 60, 61, 68, 69, 70, 73, 78, 79, 81, 88, 90, 98, 100]),
        ("data/RC208.txt", "solomon", "trunc1",
         18, 20, 34, True, False,
         99, [9, 10, 11, 12, 13, 14, 15, 16, 20, 22, 24, 47, 52, 53, 55, 56,
              57, 58, 59, 60, 64, 65, 66, 69, 74, 80, 82, 83, 86, 87, 88, 90,
              91, 98]),
        ("data/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt", "vrplib", "none",
         20, 20, 10, True, False,
         2, [5, 16, 35, 87, 174, 186, 202, 345, 389, 431]),
        ("data/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt", "vrplib", "none",
         20, 20, 10, True, True,
         2, [3, 5, 16, 35, 87, 174, 186, 202, 345, 389, 431]),
        ("data/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt", "vrplib", "none",
         18, 20, 34, True, False,
         1, [2, 6, 21, 22, 48, 60, 70, 87, 106, 119, 135, 136, 162, 165, 170,
             186, 200, 201, 202, 206, 224, 245, 261, 268, 281, 283, 316, 364,
             385, 397, 400, 415, 436, 445]),
        ("data/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt", "vrplib", "none",
         18, 20, 34, True, False,
         99, [44, 62, 65, 66, 69, 125, 141, 160, 172, 180, 215, 220, 251, 256,
              266, 267, 271, 308, 338, 346, 351, 365, 368, 369, 374, 381, 406,
              410, 419, 422, 433, 434, 440, 441]),
        ("data/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt", "vrplib", "none",
         18, 20, 34, True, True,
         98, [9, 10, 27, 38, 41, 51, 52, 55, 56, 73, 74, 76, 78, 82, 83, 88,
              113, 116, 120, 131, 143, 146, 151, 157, 158, 161, 177, 178, 192,
              199, 216, 221, 236, 239, 242, 250, 290, 325, 330, 339, 344, 359,
              360, 383, 399, 425, 438, 444]),
        ("data/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt", "vrplib", "none",
         18, 20, 34, False, True,
         98, [9, 10, 27, 38, 41, 49, 51, 52, 55, 56, 63, 73, 74, 76, 78, 82,
              83, 88, 90, 113, 116, 120, 131, 143, 146, 151, 157, 158, 161,
              177, 178, 192, 199, 216, 221, 236, 239, 242, 250, 284, 290, 325,
              330, 339, 344, 359, 360, 377, 378, 383, 391, 399, 425, 438, 444
              ]),
        ("data/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt", "vrplib", "none",
         18, 20, 34, False, False,
         98, [41, 49, 51, 52, 55, 56, 73, 74, 78, 82, 83, 88, 113, 116, 120,
              131, 143, 146, 158, 177, 192, 199, 221, 242, 290, 325, 330, 339,
              359, 360, 377, 391, 399, 444]),
        # fmt: on
    ],
)
def test_compute_neighbours(
    data_loc: str,
    instance_format: str,
    round_func: str,
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    symmetric_proximity: bool,
    symmetric_neighbours: bool,
    idx_check: int,
    expected_neighbours_check: List[int],
):
    data = read(data_loc, instance_format, round_func=round_func)
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

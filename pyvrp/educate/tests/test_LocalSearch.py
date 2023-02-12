from numpy.testing import assert_raises
from pytest import mark

from pyvrp.educate import LocalSearchParams


@mark.parametrize(
    "weight_wait_time,"
    "weight_time_warp,"
    "nb_granular,"
    "post_process_path_length",
    [
        (20, 20, 0, 1),  # empty neighbourhood structure (nbGranular)
    ],
)
def test_local_search_params_raises_for_invalid_arguments(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    post_process_path_length: int,
):
    with assert_raises(ValueError):
        LocalSearchParams(
            weight_wait_time,
            weight_time_warp,
            nb_granular,
            post_process_path_length,
        )


@mark.parametrize(
    "weight_wait_time,"
    "weight_time_warp,"
    "nb_granular,"
    "post_process_path_length",
    [
        (20, 20, 1, 1),  # non-empty neighbourhood structure (nbGranular)
        (20, 20, 1, 0),  # zero post processing should be OK
        (0, 0, 1, 5),  # no weights for wait time or time warp should be OK
    ],
)
def test_local_search_params_does_not_raise_for_valid_arguments(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
    post_process_path_length: int,
):
    LocalSearchParams(
        weight_wait_time,
        weight_time_warp,
        nb_granular,
        post_process_path_length,
    )

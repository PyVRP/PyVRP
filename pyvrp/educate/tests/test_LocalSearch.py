from pytest import mark

from pyvrp.educate import LocalSearchParams


@mark.parametrize(
    "post_process_path_length",
    [
        1,  # non-empty neighbourhood structure (nbGranular)
        0,  # zero post processing should be OK
        5,  # no weights for wait time or time warp should be OK
    ],
)
def test_local_search_params_does_not_raise_for_valid_arguments(
    post_process_path_length: int,
):
    LocalSearchParams(post_process_path_length)

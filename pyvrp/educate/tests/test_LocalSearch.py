from numpy.testing import assert_raises
from pytest import mark

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.educate import LocalSearch, LocalSearchParams
from pyvrp.tests.helpers import read


@mark.parametrize(
    "post_process_path_length",
    [
        1,  # non-empty neighbourhood structure (nbGranular)
        0,  # zero post processing should be OK
    ],
)
def test_local_search_params_does_not_raise_for_valid_arguments(
    post_process_path_length: int,
):
    LocalSearchParams(post_process_path_length)


def test_local_search_search_raises_when_there_are_no_node_operators():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    ls = LocalSearch(data, pm, rng)
    individual = Individual(data, pm, rng)

    with assert_raises(RuntimeError):
        ls.search(individual)

    # TODO intensify without route ops should also raise

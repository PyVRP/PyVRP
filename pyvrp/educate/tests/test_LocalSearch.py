from numpy.testing import assert_raises
from pytest import mark

from pyvrp import Individual, PenaltyManager, XorShift128
from pyvrp.educate import LocalSearch, LocalSearchParams
from pyvrp.tests.helpers import read


@mark.parametrize(
    "weight_wait_time, weight_time_warp, nb_granular",
    [
        (20, 20, 0),  # empty neighbourhood structure (nbGranular)
    ],
)
def test_local_search_params_raises_for_invalid_arguments(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
):
    with assert_raises(ValueError):
        LocalSearchParams(
            weight_wait_time,
            weight_time_warp,
            nb_granular,
        )


@mark.parametrize(
    "weight_wait_time, weight_time_warp, nb_granular",
    [
        (20, 20, 1),  # non-empty neighbourhood structure (nbGranular)
        (0, 0, 1),  # no weights for wait time or time warp should be OK
    ],
)
def test_local_search_params_does_not_raise_for_valid_arguments(
    weight_wait_time: int,
    weight_time_warp: int,
    nb_granular: int,
):
    LocalSearchParams(
        weight_wait_time,
        weight_time_warp,
        nb_granular,
    )


def test_local_search_raises_when_there_are_no_operators():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)
    rng = XorShift128(seed=42)

    ls = LocalSearch(data, pm, rng)
    individual = Individual(data, pm, rng)

    with assert_raises(RuntimeError):
        ls.search(individual)

    with assert_raises(RuntimeError):
        ls.intensify(individual)

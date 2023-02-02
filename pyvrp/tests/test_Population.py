from numpy.testing import assert_almost_equal, assert_equal, assert_raises
from pytest import mark

from pyvrp import PopulationParams


@mark.parametrize(
    "min_pop_size,"
    "generation_size,"
    "nb_elite,"
    "nb_close,"
    "lb_diversity,"
    "ub_diversity",
    [
        (0, 1, 1, 1, 0.0, 1.0),  # zero min_pop_size
        (-1, 1, 1, 1, 0.0, 1.0),  # -1 min_pop_size
        (1, -1, 1, 1, 0.0, 1.0),  # -1 generation_size
        (1, 1, -1, 1, 0.0, 1.0),  # -1 nb_elite
        (1, 1, 1, -1, 0.0, 1.0),  # -1 nb_close
        (1, 1, 1, -1, -1, 1.0),  # -1 lb_diversity
        (1, 1, 1, -1, 2, 1.0),  # 2 lb_diversity
        (1, 1, 1, -1, 0, -1.0),  # -1 ub_diversity
        (1, 1, 1, -1, 0, 2.0),  # 2 ub_diversity
        (1, 1, 1, -1, 1, 0.5),  # ub_diversity < lb_diversity
        (1, 1, 1, -1, 0.5, 0.5),  # ub_diversity == lb_diversity
    ],
)
def test_params_constructor_throws_when_arguments_invalid(
    min_pop_size: int,
    generation_size: int,
    nb_elite: int,
    nb_close: int,
    lb_diversity: float,
    ub_diversity: float,
):
    """
    Tests that invalid configurations are not accepted.
    """
    with assert_raises(ValueError):
        PopulationParams(
            min_pop_size,
            generation_size,
            nb_elite,
            nb_close,
            lb_diversity,
            ub_diversity,
        )


@mark.parametrize(
    "min_pop_size,"
    "generation_size,"
    "nb_elite,"
    "nb_close,"
    "lb_diversity,"
    "ub_diversity",
    [
        (1, 1, 1, 1, 0.0, 0.5),  # >0 min_pop_size
        (1, 0, 1, 1, 0.0, 0.5),  # 0 generation_size
        (1, 1, 0, 1, 0.0, 0.5),  # 0 nb_elite
        (1, 1, 1, 0, 0.0, 0.5),  # 0 nb_close
        (1, 1, 1, 1, 0.0, 0.5),  # 0 lb_diversity
        (1, 1, 1, 1, 0.0, 1.0),  # 1 ub_diversity
    ],
)
def test_params_constructor_does_not_raise_when_arguments_valid(
    min_pop_size: int,
    generation_size: int,
    nb_elite: int,
    nb_close: int,
    lb_diversity: float,
    ub_diversity: float,
):
    """
    Tests valid boundary cases.
    """
    params = PopulationParams(
        min_pop_size,
        generation_size,
        nb_elite,
        nb_close,
        lb_diversity,
        ub_diversity,
    )

    assert_equal(params.min_pop_size, min_pop_size)
    assert_equal(params.generation_size, generation_size)
    assert_equal(params.nb_elite, nb_elite)
    assert_equal(params.nb_close, nb_close)
    assert_almost_equal(params.lb_diversity, lb_diversity)
    assert_almost_equal(params.ub_diversity, ub_diversity)
    assert_equal(params.max_pop_size, min_pop_size + generation_size)


# TODO functional tests

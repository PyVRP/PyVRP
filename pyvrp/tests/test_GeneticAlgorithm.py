from numpy.testing import assert_almost_equal, assert_equal, assert_raises
from pytest import mark

from pyvrp import GeneticAlgorithmParams


@mark.parametrize(
    "nb_penalty_management,"
    "repair_probability,"
    "collect_statistics,"
    "should_intensify",
    [
        (-1, 0.0, True, True),  # nb_penalty_management < 0
        (0, -0.25, True, True),  # repair_probability < 0
        (-1, 1.25, True, True),  # repair_probability > 1
    ],
)
def test_params_constructor_throws_when_arguments_invalid(
    nb_penalty_management: int,
    repair_probability: float,
    collect_statistics: bool,
    should_intensify: bool,
):
    """
    Tests that invalid configurations are not accepted.
    """
    with assert_raises(ValueError):
        GeneticAlgorithmParams(
            nb_penalty_management,
            repair_probability,
            collect_statistics,
            should_intensify,
        )


@mark.parametrize(
    "nb_penalty_management,"
    "repair_probability,"
    "collect_statistics,"
    "should_intensify",
    [
        (0, 0.0, True, True),  # nb_penalty_management == 0
        (1, 0.0, True, True),  # repair_probability == 0
        (1, 1.0, True, True),  # repair_probability == 1
        (1, 0.5, False, True),  # collect_statistics is False
        (1, 0.5, True, False),  # should_intensify is False
        (1, 0.5, False, False),  # both False
    ],
)
def test_params_constructor_does_not_raise_when_arguments_valid(
    nb_penalty_management: int,
    repair_probability: float,
    collect_statistics: bool,
    should_intensify: bool,
):
    """
    Tests valid boundary cases.
    """
    params = GeneticAlgorithmParams(
        nb_penalty_management,
        repair_probability,
        collect_statistics,
        should_intensify,
    )

    assert_equal(params.nb_penalty_management, nb_penalty_management)
    assert_almost_equal(params.repair_probability, repair_probability)
    assert_equal(params.collect_statistics, collect_statistics)
    assert_equal(params.should_intensify, should_intensify)


# TODO functional tests

# TODO test statistics collection on Result.has_statistics

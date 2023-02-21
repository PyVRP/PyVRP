from numpy.testing import assert_almost_equal, assert_equal, assert_raises
from pytest import mark

from pyvrp import GeneticAlgorithmParams


@mark.parametrize(
    "repair_probability,"
    "collect_statistics,"
    "intensification_probability,"
    "intensify_on_best,"
    "nb_iter_no_improvement",
    [
        (-0.25, True, 0.5, True, 0),  # repair_probability < 0
        (1.25, True, 0.5, True, 0),  # repair_probability > 1
        (0.0, True, 0.5, True, -1),  # nb_iter_no_improvement < 0
    ],
)
def test_params_constructor_throws_when_arguments_invalid(
    repair_probability: float,
    collect_statistics: bool,
    intensification_probability: int,
    intensify_on_best: bool,
    nb_iter_no_improvement: int,
):
    """
    Tests that invalid configurations are not accepted.
    """
    with assert_raises(ValueError):
        GeneticAlgorithmParams(
            repair_probability,
            collect_statistics,
            intensification_probability,
            intensify_on_best,
            nb_iter_no_improvement,
        )


@mark.parametrize(
    "repair_probability,"
    "collect_statistics,"
    "intensification_probability,"
    "intensify_on_best,"
    "nb_iter_no_improvement",
    [
        (0.0, True, 0.5, True, 0),  # nb_iter_no_improvement == 0
        (0.0, True, 0.5, True, 1),  # repair_probability == 0
        (1.0, True, 0.5, True, 1),  # repair_probability == 1
        (0.5, False, 0.5, True, 1),  # collect_statistics is False
        (0.5, True, 0, True, 1),  # intensification_probability == 0
        (0.5, True, 1, True, 1),  # intensification_probability == 1
        (0.5, True, 0.5, False, 1),  # intensify_on_best is False
        (0.5, False, 0.5, False, 1),  # both False
    ],
)
def test_params_constructor_does_not_raise_when_arguments_valid(
    repair_probability: float,
    collect_statistics: bool,
    intensification_probability: int,
    intensify_on_best: bool,
    nb_iter_no_improvement: int,
):
    """
    Tests valid boundary cases.
    """
    params = GeneticAlgorithmParams(
        repair_probability,
        collect_statistics,
        intensification_probability,
        intensify_on_best,
        nb_iter_no_improvement,
    )

    assert_almost_equal(params.repair_probability, repair_probability)
    assert_equal(params.collect_statistics, collect_statistics)
    assert_equal(params.intensification_probability, intensify_on_best)
    assert_equal(params.intensify_on_best, intensify_on_best)
    assert_equal(params.nb_iter_no_improvement, nb_iter_no_improvement)


# TODO functional tests

# TODO test statistics collection on Result.has_statistics

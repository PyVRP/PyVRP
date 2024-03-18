import pathlib

from numpy.testing import assert_equal

from pyvrp.Config import Config
from pyvrp.GeneticAlgorithm import GeneticAlgorithmParams
from pyvrp.PenaltyManager import PenaltyParams
from pyvrp.Population import PopulationParams
from pyvrp.search import (
    NODE_OPERATORS,
    ROUTE_OPERATORS,
    Exchange10,
    NeighbourhoodParams,
    SwapStar,
    SwapTails,
)


def test_default_attributes():
    """
    Tests that the default attributes are set correctly.
    """
    config = Config()

    assert_equal(config.genetic, GeneticAlgorithmParams())
    assert_equal(config.penalty, PenaltyParams())
    assert_equal(config.population, PopulationParams())
    assert_equal(config.neighbourhood, NeighbourhoodParams())
    assert_equal(config.node_ops, NODE_OPERATORS)
    assert_equal(config.route_ops, ROUTE_OPERATORS)


def test_config_from_file():
    """
    Tests that the configuration is correctly loaded from a TOML file.
    """
    config_path = pathlib.Path(__file__).parent / "data" / "test_config.toml"
    config = Config.from_file(config_path)

    genetic_params = GeneticAlgorithmParams(0.1, 200)
    penalty_params = PenaltyParams(20, 0, 0, 12, 100, 1.25, 0.85, 0.43)
    population_params = PopulationParams(10, 20, 3, 4, 0.0, 1.0)
    neighbourhood_params = NeighbourhoodParams(0, 0, 20, True, True)
    node_ops = [Exchange10, SwapTails]
    route_ops = [SwapStar]

    assert_equal(config.genetic, genetic_params)
    assert_equal(config.penalty, penalty_params)
    assert_equal(config.population, population_params)
    assert_equal(config.neighbourhood, neighbourhood_params)
    assert_equal(config.node_ops, node_ops)
    assert_equal(config.route_ops, route_ops)

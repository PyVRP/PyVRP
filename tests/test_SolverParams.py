import pathlib

from numpy.testing import assert_equal

from pyvrp.GeneticAlgorithm import GeneticAlgorithmParams
from pyvrp.PenaltyManager import PenaltyParams
from pyvrp.Population import PopulationParams
from pyvrp.SolveParams import SolveParams
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
    params = SolveParams()

    assert_equal(params.genetic, GeneticAlgorithmParams())
    assert_equal(params.penalty, PenaltyParams())
    assert_equal(params.population, PopulationParams())
    assert_equal(params.neighbourhood, NeighbourhoodParams())
    assert_equal(params.node_ops, NODE_OPERATORS)
    assert_equal(params.route_ops, ROUTE_OPERATORS)


def test_solve_params_from_file():
    """
    Tests that the solver parameters are correctly loaded from a TOML file.
    """
    loc = pathlib.Path(__file__).parent / "data" / "test_config.toml"
    params = SolveParams.from_file(loc)

    genetic = GeneticAlgorithmParams(0.1, 200)
    penalty = PenaltyParams(20, 0, 0, 12, 100, 1.25, 0.85, 0.43)
    population = PopulationParams(10, 20, 3, 4, 0.0, 1.0)
    neighbourhood = NeighbourhoodParams(0, 0, 20, True, True)
    node_ops = [Exchange10, SwapTails]
    route_ops = [SwapStar]

    assert_equal(params.genetic, genetic)
    assert_equal(params.penalty, penalty)
    assert_equal(params.population, population)
    assert_equal(params.neighbourhood, neighbourhood)
    assert_equal(params.node_ops, node_ops)
    assert_equal(params.route_ops, route_ops)

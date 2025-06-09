import numpy as np
from numpy.testing import assert_, assert_equal

from pyvrp.IteratedLocalSearch import IteratedLocalSearchParams
from pyvrp.PenaltyManager import PenaltyParams
from pyvrp.accept.MovingBestAverageThreshold import (
    MovingBestAverageThresholdParams,
)
from pyvrp.search import (
    NODE_OPERATORS,
    PERTURBATION_OPERATORS,
    ROUTE_OPERATORS,
    Exchange10,
    NeighbourRemoval,
    NeighbourhoodParams,
    SwapStar,
    SwapTails,
)
from pyvrp.solve import SolveParams, solve
from pyvrp.stop import MaxIterations
from tests.helpers import DATA_DIR


def test_default_values():
    """
    Tests that the default values are set correctly.
    """
    params = SolveParams()

    assert_equal(params.ils, IteratedLocalSearchParams())
    assert_equal(params.penalty, PenaltyParams())
    assert_equal(params.neighbourhood, NeighbourhoodParams())
    assert_equal(params.node_ops, NODE_OPERATORS)
    assert_equal(params.route_ops, ROUTE_OPERATORS)
    assert_equal(params.perturbation_ops, PERTURBATION_OPERATORS)


def test_solve_params_from_file():
    """
    Tests that the solver parameters are correctly loaded from a TOML file.
    """
    params = SolveParams.from_file(DATA_DIR / "test_config.toml")

    ils = IteratedLocalSearchParams(10)
    penalty = PenaltyParams(12, 100, 1.25, 0.85, 0.43)
    neighbourhood = NeighbourhoodParams(0, 0, 20, True, True)
    mbat = MovingBestAverageThresholdParams(1, 100, None, 10000)
    node_ops = [Exchange10, SwapTails]
    route_ops = [SwapStar]
    perturbation_ops = [NeighbourRemoval]

    assert_equal(params.ils, ils)
    assert_equal(params.penalty, penalty)
    assert_equal(params.neighbourhood, neighbourhood)
    assert_equal(params.mbat, mbat)
    assert_equal(params.node_ops, node_ops)
    assert_equal(params.route_ops, route_ops)
    assert_equal(params.perturbation_ops, perturbation_ops)


def test_solve_params_from_file_defaults():
    """
    Tests that if the TOML file does not contain all solver parameters,
    it defaults to the constructor's default values.
    """
    params = SolveParams.from_file(DATA_DIR / "empty_config.toml")
    assert_equal(params, SolveParams())


def test_solve_same_seed(ok_small):
    """
    Smoke test that checks that that solving an instance with the same seed
    results in the same search trajectories.
    """
    res1 = solve(ok_small, stop=MaxIterations(10), seed=0)
    res2 = solve(ok_small, stop=MaxIterations(10), seed=0)

    assert_equal(res1.best, res2.best)
    assert_equal(res1.stats.data, res2.stats.data)


def test_solve_custom_params(rc208):
    """
    Tests that solving an instance with custom solver parameters works as
    expected by checking that only improving solutions are accepted.
    """
    # First solve using default parameters.
    res = solve(rc208, stop=MaxIterations(20))

    # Skip the first datum because it's the empty initial solution with cost 0.
    costs = [datum.current_cost for datum in res.stats.data[1:]]

    def monotonically_decreasing(arr) -> np.bool:
        return np.all(np.diff(arr) <= 0)

    # Default parameters allow accepting worse solutions, so the current costs
    # won't necessarily be monotonically decreasing.
    assert_(not monotonically_decreasing(costs))

    # Now configure MBAT to only accept improving solutions by setting
    # ``initial_weight`` to zero.
    params = SolveParams(mbat=MovingBestAverageThresholdParams(0))
    res = solve(rc208, stop=MaxIterations(20), params=params)

    costs = [datum.current_cost for datum in res.stats.data[1:]]
    assert_(monotonically_decreasing(costs))

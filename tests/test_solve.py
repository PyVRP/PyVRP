import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal

from pyvrp.IteratedLocalSearch import IteratedLocalSearchParams
from pyvrp.PenaltyManager import PenaltyParams
from pyvrp.search import (
    NODE_OPERATORS,
    PERTURBATION_OPERATORS,
    ROUTE_OPERATORS,
    Exchange10,
    NeighbourhoodParams,
    RemoveNeighbours,
    SwapStar,
    SwapTails,
)
from pyvrp.solve import SolveParams, solve
from pyvrp.stop import MaxIterations
from tests.helpers import DATA_DIR, read_solution


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
    assert_equal(params.num_perturbations, 25)
    assert_allclose(params.display_interval, 5.0)


def test_solve_params_from_file():
    """
    Tests that the solver parameters are correctly loaded from a TOML file.
    """
    params = SolveParams.from_file(DATA_DIR / "test_config.toml")

    ils = IteratedLocalSearchParams(10, 0.1, 1)
    penalty = PenaltyParams(100, 1.25, 0.85, 0.43)
    neighbourhood = NeighbourhoodParams(0, 0, 20, True, True)
    node_ops = [Exchange10, SwapTails]
    route_ops = [SwapStar]
    perturbation_ops = [RemoveNeighbours]

    assert_equal(params.ils, ils)
    assert_equal(params.penalty, penalty)
    assert_equal(params.neighbourhood, neighbourhood)
    assert_equal(params.node_ops, node_ops)
    assert_equal(params.route_ops, route_ops)
    assert_equal(params.perturbation_ops, perturbation_ops)
    assert_equal(params.num_perturbations, 10)
    assert_allclose(params.display_interval, 10.0)


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


def test_solve_initial_solution(rc208):
    """
    Tests that solving an instance with an initial solution works as
    expected by checking that the best solution found is the same as the
    initial solution.
    """
    bks = read_solution("data/RC208.sol", rc208)
    res = solve(rc208, stop=MaxIterations(0), initial_solution=bks)
    assert_equal(res.best, bks)


def test_solve_custom_params(rc208):
    """
    Tests that solving an instance with custom solver parameters works as
    expected by checking how solutions are accepted.
    """

    def monotonically_decreasing(arr) -> np.bool_:
        return np.all(np.diff(arr) <= 0)

    # First solve with ``history_length=1``, which means that all candidate
    # solutions will be accepted.
    params = SolveParams(IteratedLocalSearchParams(history_length=1))
    res = solve(rc208, stop=MaxIterations(20), params=params)

    # Because we accept all candidate solutions, the current costs won't
    # necessarily be monotonically decreasing.
    costs = [datum.current_cost for datum in res.stats]
    assert_(not monotonically_decreasing(costs))

    # Now configure ILS to only accept improving solutions by setting
    # ``initial_accept_weight=0``.
    params = SolveParams(IteratedLocalSearchParams(initial_accept_weight=0))
    res = solve(rc208, stop=MaxIterations(20), params=params)

    # The current costs should now be monotonically decreasing. The first datum
    # is skipped because it's an empty initial solution with penalised cost 0.
    costs = [datum.current_cost for datum in res.stats.data[1:]]
    assert_(monotonically_decreasing(costs))

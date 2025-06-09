from __future__ import annotations

from copy import copy
from typing import TYPE_CHECKING

import tomli

import pyvrp.search
from pyvrp.IteratedLocalSearch import (
    IteratedLocalSearch,
    IteratedLocalSearchParams,
)
from pyvrp.PenaltyManager import PenaltyManager, PenaltyParams
from pyvrp._pyvrp import ProblemData, RandomNumberGenerator, Solution
from pyvrp.accept import (
    MovingBestAverageThreshold,
    MovingBestAverageThresholdParams,
)
from pyvrp.search import (
    NODE_OPERATORS,
    PERTURBATION_OPERATORS,
    ROUTE_OPERATORS,
    LocalSearch,
    NeighbourhoodParams,
    NodeOperator,
    PerturbationOperator,
    RouteOperator,
    compute_neighbours,
)
from pyvrp.stop import MaxIterations, MaxRuntime

if TYPE_CHECKING:
    import pathlib

    from pyvrp.Result import Result
    from pyvrp.stop import StoppingCriterion


class SolveParams:
    """
    Solver parameters for PyVRP's iterated local search algorithm.

    Parameters
    ----------
    ils
        Iterated local search parameters.
    penalty
        Penalty parameters.
    neighbourhood
        Neighbourhood parameters.
    mbat
        Moving best average threshold parameters.
    node_ops
        Node operators to use in the search.
    route_ops
        Route operators to use in the search.
    perturbation_ops
        Perturbation operators to use in the search.
    """

    def __init__(
        self,
        ils: IteratedLocalSearchParams = IteratedLocalSearchParams(),
        penalty: PenaltyParams = PenaltyParams(),
        neighbourhood: NeighbourhoodParams = NeighbourhoodParams(),
        mbat: MovingBestAverageThresholdParams = (
            MovingBestAverageThresholdParams()
        ),
        node_ops: list[type[NodeOperator]] = NODE_OPERATORS,
        route_ops: list[type[RouteOperator]] = ROUTE_OPERATORS,
        perturbation_ops: list[
            type[PerturbationOperator]
        ] = PERTURBATION_OPERATORS,
    ):
        self._ils = ils
        self._penalty = penalty
        self._neighbourhood = neighbourhood
        self._mbat = mbat
        self._node_ops = node_ops
        self._route_ops = route_ops
        self._perturbation_ops = perturbation_ops

    def __eq__(self, other: object) -> bool:
        return (
            isinstance(other, SolveParams)
            and self.ils == other.ils
            and self.penalty == other.penalty
            and self.neighbourhood == other.neighbourhood
            and self.mbat == other.mbat
            and self.node_ops == other.node_ops
            and self.route_ops == other.route_ops
            and self.perturbation_ops == other.perturbation_ops
        )

    @property
    def ils(self):
        return self._ils

    @property
    def penalty(self):
        return self._penalty

    @property
    def neighbourhood(self):
        return self._neighbourhood

    @property
    def mbat(self):
        return self._mbat

    @property
    def node_ops(self):
        return self._node_ops

    @property
    def route_ops(self):
        return self._route_ops

    @property
    def perturbation_ops(self):
        return self._perturbation_ops

    @classmethod
    def from_file(cls, loc: str | pathlib.Path):
        """
        Loads the solver parameters from a TOML file.
        """
        with open(loc, "rb") as fh:
            data = tomli.load(fh)

        ils_params = IteratedLocalSearchParams(**data.get("ils", {}))
        pen_params = PenaltyParams(**data.get("penalty", {}))
        nb_params = NeighbourhoodParams(**data.get("neighbourhood", {}))
        mbat_params = MovingBestAverageThresholdParams(**data.get("mbat", {}))

        node_ops = NODE_OPERATORS
        if "node_ops" in data:
            node_ops = [getattr(pyvrp.search, op) for op in data["node_ops"]]

        route_ops = ROUTE_OPERATORS
        if "route_ops" in data:
            route_ops = [getattr(pyvrp.search, op) for op in data["route_ops"]]

        perturbation_ops = PERTURBATION_OPERATORS
        if "perturbation_ops" in data:
            perturbation_ops = [
                getattr(pyvrp.search, op) for op in data["perturbation_ops"]
            ]

        return cls(
            ils_params,
            pen_params,
            nb_params,
            mbat_params,
            node_ops,
            route_ops,
            perturbation_ops,
        )


def solve(
    data: ProblemData,
    stop: StoppingCriterion,
    seed: int = 0,
    collect_stats: bool = True,
    display: bool = False,
    params: SolveParams = SolveParams(),
) -> Result:
    """
    Solves the given problem data instance.

    Parameters
    ----------
    data
        Problem data instance to solve.
    stop
        Stopping criterion to use.
    seed
        Seed value to use for the random number stream. Default 0.
    collect_stats
        Whether to collect statistics about the solver's progress. Default
        ``True``.
    display
        Whether to display information about the solver progress. Default
        ``False``. Progress information is only available when
        ``collect_stats`` is also set, which it is by default.
    params
        Solver parameters to use. If not provided, a default will be used.

    Returns
    -------
    Result
        A Result object, containing statistics (if collected) and the best
        found solution.
    """
    rng = RandomNumberGenerator(seed=seed)
    neighbours = compute_neighbours(data, params.neighbourhood)
    ls = LocalSearch(data, rng, neighbours)

    for node_op in params.node_ops:
        if node_op.supports(data):
            ls.add_node_operator(node_op(data))

    for route_op in params.route_ops:
        if route_op.supports(data):
            ls.add_route_operator(route_op(data))

    for perturb_op in params.perturbation_ops:
        ls.add_perturbation_operator(perturb_op(data, 10))

    # In case the convergence parameters are not set for MBAT, infer them from
    # the stopping criterion.
    mbat = copy(params.mbat)
    if mbat.max_runtime is None:
        mbat.max_runtime = _stop2runtime(stop)

    if mbat.max_iterations is None:
        mbat.max_iterations = _stop2iterations(stop)

    accept = MovingBestAverageThreshold(**vars(mbat))
    pm = PenaltyManager.init_from(data, params.penalty)
    init = Solution(data, [])  # type: ignore

    ils_args = (data, pm, rng, ls, accept, init, params.ils)
    algo = IteratedLocalSearch(*ils_args)  # type: ignore
    return algo.run(stop, collect_stats, display)


def _stop2runtime(stop) -> float | None:
    return stop.max_runtime if isinstance(stop, MaxRuntime) else None


def _stop2iterations(stop) -> int | None:
    return stop.max_iterations if isinstance(stop, MaxIterations) else None

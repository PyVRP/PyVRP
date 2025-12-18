from __future__ import annotations

import tomllib
from typing import TYPE_CHECKING

import pyvrp.search
from pyvrp.IteratedLocalSearch import (
    IteratedLocalSearch,
    IteratedLocalSearchParams,
)
from pyvrp.PenaltyManager import PenaltyManager, PenaltyParams
from pyvrp._pyvrp import ProblemData, RandomNumberGenerator, Solution
from pyvrp.search import (
    NODE_OPERATORS,
    ROUTE_OPERATORS,
    LocalSearch,
    NeighbourhoodParams,
    NodeOperator,
    PerturbationManager,
    PerturbationParams,
    RouteOperator,
    compute_neighbours,
)

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
    node_ops
        Node operators to use in the search.
    route_ops
        Route operators to use in the search.
    display_interval
        Time (in seconds) between iteration logs. Default 5s.
    perturbation
        Perturbation parameters.
    initial_solution
        Solution to start the search from. If not provided, a default solution
        will be created.
    """

    def __init__(
        self,
        ils: IteratedLocalSearchParams = IteratedLocalSearchParams(),
        penalty: PenaltyParams = PenaltyParams(),
        neighbourhood: NeighbourhoodParams = NeighbourhoodParams(),
        node_ops: list[type[NodeOperator]] = NODE_OPERATORS,
        route_ops: list[type[RouteOperator]] = ROUTE_OPERATORS,
        display_interval: float = 5.0,
        perturbation: PerturbationParams = PerturbationParams(),
        initial_solution: Solution | None = None,
    ):
        self._ils = ils
        self._penalty = penalty
        self._neighbourhood = neighbourhood
        self._node_ops = node_ops
        self._route_ops = route_ops
        self._display_interval = display_interval
        self._perturbation = perturbation
        self._initial_solution = initial_solution

    def __eq__(self, other: object) -> bool:
        return (
            isinstance(other, SolveParams)
            and self.ils == other.ils
            and self.penalty == other.penalty
            and self.neighbourhood == other.neighbourhood
            and self.node_ops == other.node_ops
            and self.route_ops == other.route_ops
            and self.display_interval == other.display_interval
            and self.perturbation == other.perturbation
            and self.initial_solution == other.initial_solution
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
    def node_ops(self):
        return self._node_ops

    @property
    def route_ops(self):
        return self._route_ops

    @property
    def display_interval(self) -> float:
        return self._display_interval

    @property
    def perturbation(self):
        return self._perturbation

    @property
    def initial_solution(self) -> Solution | None:
        return self._initial_solution

    @classmethod
    def from_file(cls, loc: str | pathlib.Path):
        """
        Loads the solver parameters from a TOML file.

        .. note::
           The initial solution cannot be loaded from file, and will always be
           set to ``None``.
        """
        with open(loc, "rb") as fh:
            data = tomllib.load(fh)

        node_ops = NODE_OPERATORS
        if "node_ops" in data:
            node_ops = [getattr(pyvrp.search, op) for op in data["node_ops"]]

        route_ops = ROUTE_OPERATORS
        if "route_ops" in data:
            route_ops = [getattr(pyvrp.search, op) for op in data["route_ops"]]

        return cls(
            IteratedLocalSearchParams(**data.get("ils", {})),
            PenaltyParams(**data.get("penalty", {})),
            NeighbourhoodParams(**data.get("neighbourhood", {})),
            node_ops,
            route_ops,
            data.get("display_interval", 5.0),
            PerturbationParams(**data.get("perturbation", {})),
            None,  # initial solution cannot be loaded from file
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
    perturbation = PerturbationManager(params.perturbation)
    ls = LocalSearch(data, rng, neighbours, perturbation)

    for node_op in params.node_ops:
        if node_op.supports(data):
            ls.add_node_operator(node_op(data))

    for route_op in params.route_ops:
        if route_op.supports(data):
            ls.add_route_operator(route_op(data))

    pm = PenaltyManager.init_from(data, params.penalty)

    init = params.initial_solution
    if init is None:
        init = ls(Solution(data, []), pm.max_cost_evaluator())

    algo = IteratedLocalSearch(data, pm, rng, ls, init, params.ils)
    return algo.run(stop, collect_stats, display, params.display_interval)

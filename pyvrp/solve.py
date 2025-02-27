from __future__ import annotations

from typing import TYPE_CHECKING, Type, Union

import tomli

import pyvrp.search
from pyvrp.IteratedLocalSearch import (
    IteratedLocalSearch,
    IteratedLocalSearchParams,
)
from pyvrp.PenaltyManager import PenaltyManager, PenaltyParams
from pyvrp._pyvrp import ProblemData, RandomNumberGenerator, Solution
from pyvrp.accept import MovingAverageThreshold
from pyvrp.search import (
    NODE_OPERATORS,
    ROUTE_OPERATORS,
    DestroyRepair,
    LocalSearch,
    NeighbourhoodParams,
    NodeOperator,
    RouteOperator,
    compute_neighbours,
)

if TYPE_CHECKING:
    import pathlib

    from pyvrp.Result import Result
    from pyvrp.stop import StoppingCriterion


class SolveParams:
    """
    Solver parameters for PyVRP's hybrid genetic search algorithm.

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
    """

    def __init__(
        self,
        ils: IteratedLocalSearchParams = IteratedLocalSearchParams(),
        penalty: PenaltyParams = PenaltyParams(),
        neighbourhood: NeighbourhoodParams = NeighbourhoodParams(),
        node_ops: list[Type[NodeOperator]] = NODE_OPERATORS,
        route_ops: list[Type[RouteOperator]] = ROUTE_OPERATORS,
    ):
        self._ils = ils
        self._penalty = penalty
        self._neighbourhood = neighbourhood
        self._node_ops = node_ops
        self._route_ops = route_ops

    def __eq__(self, other: object) -> bool:
        return (
            isinstance(other, SolveParams)
            and self.ils == other.ils
            and self.penalty == other.penalty
            and self.neighbourhood == other.neighbourhood
            and self.node_ops == other.node_ops
            and self.route_ops == other.route_ops
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

    @classmethod
    def from_file(cls, loc: Union[str, pathlib.Path]):
        """
        Loads the solver parameters from a TOML file.
        """
        with open(loc, "rb") as fh:
            data = tomli.load(fh)

        ils_params = IteratedLocalSearchParams(**data.get("ils", {}))
        pen_params = PenaltyParams(**data.get("penalty", {}))
        nb_params = NeighbourhoodParams(**data.get("neighbourhood", {}))

        node_ops = NODE_OPERATORS
        if "node_ops" in data:
            node_ops = [getattr(pyvrp.search, op) for op in data["node_ops"]]

        route_ops = ROUTE_OPERATORS
        if "route_ops" in data:
            route_ops = [getattr(pyvrp.search, op) for op in data["route_ops"]]

        return cls(
            ils_params,
            pen_params,
            nb_params,
            node_ops,
            route_ops,
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

    VALUE = 1_000_000  # TODO Ignore the penalties for now
    penalty_params = PenaltyParams(solutions_between_updates=VALUE)
    pm = PenaltyManager(
        initial_penalties=([VALUE], VALUE, VALUE),
        params=penalty_params,
    )
    nbhd = compute_neighbours(data)
    perturb = DestroyRepair(data, rng, nbhd)
    ls = LocalSearch(data, rng, nbhd)

    for node_op in params.node_ops:
        ls.add_node_operator(node_op(data))

    for route_op in params.route_ops:
        ls.add_route_operator(route_op(data))

    accept = MovingAverageThreshold(eta=0.5, history_size=100)
    ils_args = (data, pm, rng, perturb, ls, accept, params.ils)
    algo = IteratedLocalSearch(*ils_args)  # type: ignore
    init = ls(Solution.make_random(data, rng), pm.cost_evaluator())
    return algo.run(stop, init, collect_stats, display)

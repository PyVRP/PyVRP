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
    OPERATORS,
    BinaryOperator,
    LocalSearch,
    NeighbourhoodParams,
    PerturbationManager,
    PerturbationParams,
    UnaryOperator,
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
    operators
        Operators to use in the search.
    display_interval
        Time (in seconds) between iteration logs. Default 5s.
    perturbation
        Perturbation parameters.
    """

    def __init__(
        self,
        ils: IteratedLocalSearchParams = IteratedLocalSearchParams(),
        penalty: PenaltyParams = PenaltyParams(),
        neighbourhood: NeighbourhoodParams = NeighbourhoodParams(),
        operators: list[type[UnaryOperator | BinaryOperator]] = OPERATORS,
        display_interval: float = 5.0,
        perturbation: PerturbationParams = PerturbationParams(),
    ):
        self._ils = ils
        self._penalty = penalty
        self._neighbourhood = neighbourhood
        self._operators = operators
        self._display_interval = display_interval
        self._perturbation = perturbation

    def __eq__(self, other: object) -> bool:
        return (
            isinstance(other, SolveParams)
            and self.ils == other.ils
            and self.penalty == other.penalty
            and self.neighbourhood == other.neighbourhood
            and self.operators == other.operators
            and self.display_interval == other.display_interval
            and self.perturbation == other.perturbation
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
    def operators(self):
        return self._operators

    @property
    def display_interval(self) -> float:
        return self._display_interval

    @property
    def perturbation(self):
        return self._perturbation

    @classmethod
    def from_file(cls, loc: str | pathlib.Path):
        """
        Loads the solver parameters from a TOML file.
        """
        with open(loc, "rb") as fh:
            data = tomllib.load(fh)

        operators = OPERATORS
        if "operators" in data:
            operators = [getattr(pyvrp.search, op) for op in data["operators"]]

        return cls(
            IteratedLocalSearchParams(**data.get("ils", {})),
            PenaltyParams(**data.get("penalty", {})),
            NeighbourhoodParams(**data.get("neighbourhood", {})),
            operators,
            data.get("display_interval", 5.0),
            PerturbationParams(**data.get("perturbation", {})),
        )


def solve(
    data: ProblemData,
    stop: StoppingCriterion,
    seed: int = 0,
    collect_stats: bool = True,
    display: bool = False,
    params: SolveParams = SolveParams(),
    initial_solution: Solution | None = None,
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
    initial_solution
        Optional solution to use as a warm start. The solver constructs a
        (possibly poor) initial solution if this argument is not provided.

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

    for op in params.operators:
        if op.supports(data):
            ls.add_operator(op(data))

    penalties = params.penalty.midpoint_penalties(data)
    pm = PenaltyManager(penalties, params.penalty)

    init = initial_solution
    if init is None:
        # Start from a random initial solution to ensure it's not completely
        # empty (because starting from empty solutions can be a bit difficult).
        random = Solution.make_random(data, rng)
        init = ls(random, pm.max_cost_evaluator(), exhaustive=True)

    algo = IteratedLocalSearch(data, pm, rng, ls, init, params.ils)
    return algo.run(stop, collect_stats, display, params.display_interval)

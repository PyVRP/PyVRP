from __future__ import annotations

import time
from dataclasses import dataclass
from typing import TYPE_CHECKING

from pyvrp.Result import Result

if TYPE_CHECKING:
    from pyvrp.PenaltyManager import PenaltyManager
    from pyvrp._pyvrp import (
        CostEvaluator,
        ProblemData,
        RandomNumberGenerator,
        Solution,
    )
    from pyvrp.accept import AcceptanceCriterion
    from pyvrp.search.SearchMethod import SearchMethod
    from pyvrp.stop.StoppingCriterion import StoppingCriterion


@dataclass
class _Datum:
    """
    Single ILS iteration data point.
    """

    current_cost: float
    candidate_cost: float
    best_cost: float


class Statistics:
    """
    Statistics about the Iterated Local Search progress.
    """

    def __init__(self, collect_stats: bool = True):
        self.runtimes = []
        self.num_iterations = 0
        self.stats = []

        self._clock = time.perf_counter()
        self._collect_stats = collect_stats

    def is_collecting(self) -> bool:
        return self._collect_stats

    def collect(self, current: float, candidate: float, best: float):
        """
        Collects statistics from the ILS iteration.
        """
        if not self._collect_stats:
            return

        start = self._clock
        self._clock = time.perf_counter()

        self.runtimes.append(self._clock - start)
        self.num_iterations += 1

        self.stats.append(_Datum(current, candidate, best))

        # Hacky "progress printer"
        if self.num_iterations % 50 == 0:
            print(
                "Iter: ",
                self.num_iterations,
                "Time (s): ",
                f"{sum(self.runtimes):.2f}",
                self.stats[-1],
            )


@dataclass
class IteratedLocalSearchParams:
    """
    Parameters for the iterated local search algorithm.
    """

    pass


class IteratedLocalSearch:
    """
    Creates an IteratedLocalSearch instance.

    Parameters
    ----------
    data
        The problem data instance.
    penalty_manager
        Penalty manager to use.
    rng
        Random number generator.
    perturb_method
        Perturb method to use.
    search_method
        Search method to use.
    acceptance_criterion
        Acceptance criterion to use.
    params
        Iterated local search parameters to use. If not provided, a default
        will be used.
    """

    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: RandomNumberGenerator,
        perturb_method: SearchMethod,
        search_method: SearchMethod,
        acceptance_criterion: AcceptanceCriterion,
        params: IteratedLocalSearchParams,
    ):
        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._perturb = perturb_method
        self._search = search_method
        self._accept = acceptance_criterion
        self._params = params

    @property
    def _cost_evaluator(self) -> CostEvaluator:
        return self._pm.cost_evaluator()

    def run(
        self,
        stop: StoppingCriterion,
        initial_solution: Solution,
        collect_stats: bool = True,
        display: bool = False,
    ) -> Result:
        """
        Runs the iterated local search algorithm.

        Parameters
        ----------
        stop
            Stopping criterion to use. The algorithm runs until the first time
            the stopping criterion returns ``True``.
        initial_solution
            The initial solution to use in the first iteration.
        collect_stats
            Whether to collect statistics about the solver's progress. Default
            ``True``.
        """
        best = current = initial_solution

        start = time.perf_counter()
        stats = Statistics(collect_stats=collect_stats)
        iters = 0

        while not stop(self._cost_evaluator.cost(best)):
            iters += 1

            perturbed = self._perturb(current, self._cost_evaluator)
            candidate = self._search(perturbed, self._cost_evaluator)
            self._pm.register(candidate)

            if not candidate.is_feasible():
                booster_cost_eval = self._pm.booster_cost_evaluator()
                candidate = self._search(candidate, booster_cost_eval)
                self._pm.register(candidate)

            cand_cost = self._cost_evaluator.cost(candidate)
            best_cost = self._cost_evaluator.cost(best)
            curr_cost = self._cost_evaluator.cost(current)
            stats.collect(curr_cost, cand_cost, best_cost)

            if cand_cost < best_cost:
                best, current = candidate, candidate
            elif self._accept(best_cost, curr_cost, cand_cost):
                current = candidate

        end = time.perf_counter()
        runtime = end - start

        return Result(
            best,
            stats,
            iters,
            runtime,
        )

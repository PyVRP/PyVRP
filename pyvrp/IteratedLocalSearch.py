from __future__ import annotations

import time
from dataclasses import dataclass
from typing import TYPE_CHECKING

from pyvrp.ProgressPrinter import ProgressPrinter
from pyvrp.Result import Result
from pyvrp.Statistics import Statistics

if TYPE_CHECKING:
    from pyvrp.PenaltyManager import PenaltyManager
    from pyvrp._pyvrp import (
        CostEvaluator,
        ProblemData,
        RandomNumberGenerator,
        Solution,
    )
    from pyvrp.accept.AcceptanceCriterion import AcceptanceCriterion
    from pyvrp.search.SearchMethod import SearchMethod
    from pyvrp.stop.StoppingCriterion import StoppingCriterion


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
        print_progress = ProgressPrinter(should_print=display)
        print_progress.start(self._data)

        start = time.perf_counter()
        stats = Statistics(collect_stats=collect_stats)
        iters = 0
        best = current = initial_solution

        while not stop(self._cost_evaluator.cost(best)):
            iters += 1

            perturbed = self._perturb(current, self._cost_evaluator)
            candidate = self._search(perturbed, self._cost_evaluator)
            self._pm.register(candidate)

            cand_cost = self._cost_evaluator.cost(candidate)
            best_cost = self._cost_evaluator.cost(best)
            curr_cost = self._cost_evaluator.cost(current)

            stats.collect(
                curr_cost,
                current.is_feasible(),
                cand_cost,
                candidate.is_feasible(),
                best_cost,
                best.is_feasible(),
                0,  # TODO
            )
            print_progress.iteration(stats)

            if not candidate.is_feasible():
                continue  # skip infeasible solutions for now

            if cand_cost < best_cost:
                best, current = candidate, candidate
            elif self._accept(best_cost, curr_cost, cand_cost):
                current = candidate

        runtime = time.perf_counter() - start
        res = Result(best, stats, iters, runtime)

        print_progress.end(res)

        return res

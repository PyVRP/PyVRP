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

    num_iters_no_improvement: int = 20_000

    def __post_init__(self):
        if self.num_iters_no_improvement < 0:
            raise ValueError("num_iter_no_improvement < 0 not understood.")


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
    search_method
        Search method to use.
    acceptance_criterion
        Acceptance criterion to use.
    initial_solution
        Initial solution to start the search with.
    params
        Iterated local search parameters to use. If not provided, a default
        will be used.
    """

    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: RandomNumberGenerator,
        search_method: SearchMethod,
        acceptance_criterion: AcceptanceCriterion,
        initial_solution: Solution,
        params: IteratedLocalSearchParams = IteratedLocalSearchParams(),
    ):
        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._search = search_method
        self._accept = acceptance_criterion
        self._init = initial_solution
        self._params = params

    @property
    def _cost_evaluator(self) -> CostEvaluator:
        return self._pm.cost_evaluator()

    @property
    def _booster_cost_evaluator(self) -> CostEvaluator:
        return self._pm.booster_cost_evaluator()

    def _stats(self, solution: Solution) -> tuple[int, bool]:
        penalised_cost = self._cost_evaluator.penalised_cost(solution)
        is_feasible = solution.is_feasible()
        return penalised_cost, is_feasible

    def run(
        self,
        stop: StoppingCriterion,
        collect_stats: bool = True,
        display: bool = False,
    ) -> Result:
        """
        Runs the iterated local search algorithm with the provided stopping
        criterion.

        Parameters
        ----------
        stop
            Stopping criterion to use. The algorithm runs until the first time
            the stopping criterion returns ``True``.
        collect_stats
            Whether to collect statistics about the solver's progress. Default
            ``True``.
        display
            Whether to display information about the solver progress. Default
            ``False``. Progress information is only available when
            ``collect_stats`` is also set.
        """
        print_progress = ProgressPrinter(should_print=display)
        print_progress.start(self._data)

        start = time.perf_counter()
        stats = Statistics(collect_stats=collect_stats)
        iters = 0
        iters_no_improvement = 1
        best = current = self._init

        while not stop(self._cost_evaluator.cost(best)):
            iters += 1

            if iters_no_improvement == self._params.num_iters_no_improvement:
                print_progress.restart()
                iters_no_improvement = 1
                current = best

            candidate = self._search(current, self._cost_evaluator)
            self._pm.register(candidate)

            if not candidate.is_feasible():
                candidate = self._search(
                    candidate, self._booster_cost_evaluator
                )
                self._pm.register(candidate)

            stats.collect(
                *self._stats(current),
                *self._stats(candidate),
                *self._stats(best),
            )

            cand_feas = candidate.is_feasible()
            cand_cost = self._cost_evaluator.cost(candidate)
            curr_cost = self._cost_evaluator.cost(current)
            best_cost = self._cost_evaluator.cost(best)

            if cand_feas and cand_cost < best_cost:  # new best
                best = candidate
                iters_no_improvement = 1
            else:
                iters_no_improvement += 1

            if cand_feas and self._accept(best_cost, curr_cost, cand_cost):
                current = candidate

            print_progress.iteration(stats)

        runtime = time.perf_counter() - start
        res = Result(best, stats, iters, runtime)

        print_progress.end(res)

        return res

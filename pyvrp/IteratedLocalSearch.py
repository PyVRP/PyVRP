from __future__ import annotations

import time
from dataclasses import dataclass
from typing import TYPE_CHECKING

import numpy as np

from pyvrp.ProgressPrinter import ProgressPrinter
from pyvrp.Result import Result
from pyvrp.Statistics import Statistics

if TYPE_CHECKING:
    from pyvrp.PenaltyManager import PenaltyManager
    from pyvrp._pyvrp import ProblemData, RandomNumberGenerator, Solution
    from pyvrp.search.SearchMethod import SearchMethod
    from pyvrp.stop.StoppingCriterion import StoppingCriterion


@dataclass
class IteratedLocalSearchParams:
    """
    Parameters for the iterated local search algorithm.

    Parameters
    ----------
    num_iters_no_improvement
        Number of iterations without any improvement needed before a restart
        occurs.
    history_length
        Length of the LAHC fitness array.
    """

    num_iters_no_improvement: int = 20_000
    history_length: int = 500

    def __post_init__(self):
        if self.num_iters_no_improvement < 0:
            raise ValueError("num_iters_no_improvement < 0 not understood.")

        if self.history_length <= 0:
            raise ValueError("history_length must be positive.")


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
        initial_solution: Solution,
        params: IteratedLocalSearchParams = IteratedLocalSearchParams(),
    ):
        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._search = search_method
        self._init = initial_solution
        self._params = params

    def run(
        self,
        stop: StoppingCriterion,
        collect_stats: bool = True,
        display: bool = False,
        display_interval: float = 5.0,
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
        display_interval
            Time (in seconds) between iteration logs. Defaults to 5s.

        Returns
        -------
        Result
            A Result object, containing statistics (if collected) and the best
            found solution.
        """
        print_progress = ProgressPrinter(display, display_interval)
        print_progress.start(self._data)

        stats = Statistics(collect_stats=collect_stats)

        start = time.perf_counter()
        iters = iters_no_improvement = 0
        best = current = self._init

        cost_eval = self._pm.cost_evaluator()
        history = History(self._init, self._params.history_length)

        while not stop(cost_eval.cost(best)):
            iters += 1
            iters_no_improvement += 1

            if iters_no_improvement == self._params.num_iters_no_improvement:
                print_progress.restart()
                current = best
                iters_no_improvement = 0

            cost_eval = self._pm.cost_evaluator()
            candidate = self._search(current, cost_eval)
            self._pm.register(candidate)

            if cost_eval.cost(candidate) < cost_eval.cost(best):  # new best
                best = candidate
                iters_no_improvement = 0

            # Late Acceptance Hill Climbing (LAHC) acceptance criterion from
            # Burke & Bykov (2012) with all enhancements (section 4.2).
            cand_cost = cost_eval.penalised_cost(candidate)
            curr_cost = cost_eval.penalised_cost(current)
            late_cost = history.get_late_cost(cost_eval)

            if cand_cost <= max(curr_cost, late_cost):
                current = candidate

            history.update(current, cost_eval)

            stats.collect(
                current,
                candidate,
                best,
                cost_eval,
                history.min(cost_eval),
                history.max(cost_eval),
            )
            print_progress.iteration(stats)

        runtime = time.perf_counter() - start
        res = Result(best, stats, iters, runtime)

        print_progress.end(res)

        return res


class History:
    def __init__(self, sol: Solution, size: int):
        self._array = np.full(size, sol)
        self._iter = 0

    def get_late_cost(self, cost_eval) -> float:
        """
        Returns the cost from Lh iterations ago (the current virtual position).
        """
        idx = self._iter % len(self._array)
        return cost_eval.penalised_cost(self._array[idx])

    def min(self, cost_eval) -> float:
        return min([cost_eval.penalised_cost(sol) for sol in self._array])

    def max(self, cost_eval) -> float:
        return max([cost_eval.penalised_cost(sol) for sol in self._array])

    def mean(self, cost_eval) -> float:
        return np.mean([cost_eval.penalised_cost(sol) for sol in self._array])

    def update(self, current: Solution, cost_eval):
        """
        Updates the fitness array with the current cost, but only if it's
        better (enhanced history recording from LAHC variant).
        """
        idx = self._iter % len(self._array)
        curr_cost = cost_eval.penalised_cost(current)

        if curr_cost < cost_eval.penalised_cost(self._array[idx]):
            self._array[idx] = current

        self._iter += 1

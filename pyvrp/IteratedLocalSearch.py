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
    initial_accept_weight
        Initial weight parameter used to determine the threshold value in the
        acceptance criterion. Larger values result in more accepted candidate
        solutions. Must be in [0, 1].
    history_length
        The number of recent candidate solutions to consider when computing the
        threshold value in the acceptance criterion. Must be positive.
    """

    num_iters_no_improvement: int = 20_000
    initial_accept_weight: float = 1
    history_length: int = 500
    budget: int = 20_000

    def __post_init__(self):
        if self.num_iters_no_improvement < 0:
            raise ValueError("num_iters_no_improvement < 0 not understood.")

        if not (0 <= self.initial_accept_weight <= 1):
            raise ValueError("initial_accept_weight must be in [0, 1].")

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

        history = History(size=self._params.history_length)
        stats = Statistics(collect_stats=collect_stats)

        start = time.perf_counter()
        iters = iters_no_improvement = iters_budget = 0
        best = current = self._init

        cost_eval = self._pm.cost_evaluator()
        while not stop(cost_eval.cost(best)):
            iters += 1
            iters_no_improvement += 1
            iters_budget += 1

            if iters_no_improvement == self._params.num_iters_no_improvement:
                print_progress.restart()
                history.clear()
                history.append(cost_eval.penalised_cost(best))

                current = best
                iters_no_improvement = 0

            cost_eval = self._pm.cost_evaluator()
            candidate = self._search(current, cost_eval)
            self._pm.register(candidate)

            if cost_eval.cost(candidate) < cost_eval.cost(best):  # new best
                best = candidate
                iters_no_improvement = 0

            cand_cost = cost_eval.penalised_cost(candidate)
            history.append(cand_cost)

            # Evaluate replacing the current solution with the candidate. A
            # candidate solution is accepted if it is better than a threshold
            # value based on the recent history of candidate objectives. This
            # threshold value is a convex combination of the recent best and
            # mean values. Based on Maximo and Nascimento (2021); see
            # https://doi.org/10.1016/j.ejor.2021.02.024 for more details.
            weight = self._params.initial_accept_weight
            if (fraction := stop.fraction_remaining()) is not None:
                weight *= fraction

            weight *= 1 - (iters_budget / self._params.budget)

            best_weight = (1 - weight) * history.min()
            mean_weight = weight * history.mean()
            if cand_cost <= best_weight + mean_weight:
                current = candidate

            if iters_budget >= self._params.budget:
                iters_budget = 0

            stats.collect(
                current,
                candidate,
                best,
                cost_eval,
                weight,
                best_weight + mean_weight,
            )
            print_progress.iteration(stats)

        runtime = time.perf_counter() - start
        res = Result(best, stats, iters, runtime)

        print_progress.end(res)

        return res


class History:
    """
    Small helper class to manage a history of recent candidate solution values.
    """

    def __init__(self, size: int):
        self._array = np.full(shape=(size,), fill_value=np.nan)
        self._idx = 0

    def __len__(self) -> int:
        return np.count_nonzero(~np.isnan(self._array))

    def clear(self):
        self._array.fill(np.nan)
        self._idx = 0

    def append(self, value: int):
        self._array[self._idx % self._array.size] = value
        self._idx += 1

    def min(self) -> float:
        return np.nanmin(self._array)

    def mean(self) -> float:
        return np.nanmean(self._array)

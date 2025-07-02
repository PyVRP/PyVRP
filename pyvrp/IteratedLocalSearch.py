from __future__ import annotations

import time
from collections import deque
from dataclasses import dataclass
from statistics import fmean
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

        self._history: deque[int] = deque(maxlen=self._params.history_length)

    @property
    def _cost_evaluator(self) -> CostEvaluator:
        return self._pm.cost_evaluator()

    @property
    def _booster_cost_evaluator(self) -> CostEvaluator:
        return self._pm.booster_cost_evaluator()

    def _accept(
        self, candidate: Solution, best: Solution, stop: StoppingCriterion
    ) -> bool:
        R"""
        Returns whether the candidate solution should be accepted or not.
        A candidate solution is accepted if it is feasible and better than a
        threshold value based on the objective values of recently observed
        candidate solutions. Specifically, the threshold value is a convex
        combination of the recent best and average values, computed as:

        .. math::

            (1 - w) \times f(s^*) + w \times \sum_{j = 1}^N \frac{f(s^j)}{N}

        where :math:`s^*` is the best solution observed in the last :math:`N`
        iterations, :math:`f(\cdot)` is the objective function,
        :math:`N \in \mathbb{N}` is the history length parameter, and each
        :math:`s^j` is a recently observed solution.

        The weight :math:`w` starts at initial accept weight parameter
        :math:`w_0 \in [0, 1]` and decreases proportionally to the remaining
        search time:

        .. math::

            w = w_0 \times \text{fraction remaining of stopping criterion}

        As the algorithm progresses, the threshold becomes more selective,
        transitioning from exploration (accepting more diverse solutions)
        to exploitation (accepting only improving solutions).

        .. note::

            This method is based on the acceptence criterion of [1]_. The
            parameters ``initial_accept_weight`` and ``history_length``
            correspond to :math:`\eta` and :math:`\gamma` respectively in [1]_.

        Parameters
        ----------
        candidate
            The candidate solution to consider.
        best
            The best solution found so far.
        stop
            The stopping criterion of this run.

        Returns
        -------
        bool
            True if the candidate solution should be accepted, False otherwise.

        References
        ----------
        .. [1] Máximo, V.R. and M.C.V. Nascimento. 2021. A hybrid adaptive
            iterated local search with diversification control to the
            capacitated vehicle routing problem,
            *European Journal of Operational Research* 294 (3): 1108 - 1119.
            https://doi.org/10.1016/j.ejor.2021.02.024.
        """
        if not best.is_feasible():  # then always accept candidates
            if candidate.is_feasible():
                cand_cost = self._cost_evaluator.cost(candidate)
                self._history.append(cand_cost)

            return True

        if not candidate.is_feasible():
            # Don't accept infeasible solutions and don't register their
            # cost, as it would skew the history costs.
            return False

        cand_cost = self._cost_evaluator.cost(candidate)
        self._history.append(cand_cost)

        recent_best = min(self._history)
        recent_avg = fmean(self._history)

        weight = self._params.initial_accept_weight
        if (fraction := stop.fraction_remaining()) is not None:
            weight *= fraction

        return cand_cost <= (1 - weight) * recent_best + weight * recent_avg

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
        """
        print_progress = ProgressPrinter(display, display_interval)
        print_progress.start(self._data)

        self._history.clear()
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
                self._history.clear()

            candidate = self._search(current, self._cost_evaluator)
            self._pm.register(candidate)

            if not candidate.is_feasible():
                candidate = self._search(
                    candidate, self._booster_cost_evaluator
                )
                self._pm.register(candidate)

            stats.collect(current, candidate, best, self._cost_evaluator)
            print_progress.iteration(stats)

            cand_cost = self._cost_evaluator.cost(candidate)
            best_cost = self._cost_evaluator.cost(best)

            if cand_cost < best_cost:  # new best
                best = candidate
                iters_no_improvement = 1
            else:
                iters_no_improvement += 1

            if self._accept(candidate, best, stop):
                current = candidate

        runtime = time.perf_counter() - start
        res = Result(best, stats, iters, runtime)

        print_progress.end(res)

        return res

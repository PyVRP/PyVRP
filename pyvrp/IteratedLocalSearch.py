from __future__ import annotations

import time
from dataclasses import dataclass
from typing import TYPE_CHECKING

from pyvrp.ProgressPrinter import ProgressPrinter
from pyvrp.Result import Result
from pyvrp.RingBuffer import RingBuffer
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
        Number of iterations without improvement before a restart occurs.
    history_length
        History length for the late acceptance hill-climbing stopping criterion
        used by the algorithm. Must be positive.
    """

    num_iters_no_improvement: int = 50_000
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
        criterion. The algorithm uses late acceptance hill-climbing as the
        acceptance criterion; see [1]_ for details.

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

        References
        ----------
        .. [1] Burke, E.K., and Y. Bykov (2017). The Late Acceptance
               Hill-Climbing Heuristic. *European Journal of Operational
               Research*, 258(1): 70 - 78.
               https://doi.org/10.1016/j.ejor.2016.07.012.
        """
        print_progress = ProgressPrinter(display, display_interval)
        print_progress.start(self._data)

        history: RingBuffer[Solution] = RingBuffer(self._params.history_length)
        stats = Statistics(collect_stats=collect_stats)

        start = time.perf_counter()
        iters = iters_no_improvement = 0
        best = current = self._init

        cost_eval = self._pm.cost_evaluator()
        while not stop(cost_eval.cost(best)):
            iters += 1

            if iters_no_improvement == self._params.num_iters_no_improvement:
                print_progress.restart()
                history.clear()

                current = best
                iters_no_improvement = 0

            cost_eval = self._pm.cost_evaluator()
            candidate = self._search(current, cost_eval)
            self._pm.register(candidate)

            iters_no_improvement += 1
            if cost_eval.cost(candidate) < cost_eval.cost(best):  # new best
                best = candidate
                iters_no_improvement = 0

            cand_cost = cost_eval.penalised_cost(candidate)
            curr_cost = cost_eval.penalised_cost(current)

            # We use either the current best or the current cost value from
            # some iterations ago to determine whether to accept the candidate
            # solution, if available.
            late_cost = cost_eval.penalised_cost(best)
            if (late := history.peek()) is not None:
                late_cost = cost_eval.penalised_cost(late)

            # Late-acceptance hill climbing of Burke and Bykov (2017). We use
            # both enhancements of section 4.2:
            # 1. We accept also when the candidate improves over the current
            #    solution;
            if cand_cost < late_cost or cand_cost < curr_cost:
                current = candidate
                curr_cost = cand_cost

            # 2. We update the history only when the current solution is better
            #    than the one already in the history.
            if curr_cost < late_cost or late is None:
                history.append(current)
            else:
                history.skip()

            stats.collect(current, candidate, best, cost_eval)
            print_progress.iteration(stats)

        runtime = time.perf_counter() - start
        res = Result(best, stats, iters, runtime)

        print_progress.end(res)

        return res

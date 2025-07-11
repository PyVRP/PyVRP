from __future__ import annotations

import time
from dataclasses import dataclass
from typing import TYPE_CHECKING

from pyvrp.ProgressPrinter import ProgressPrinter
from pyvrp.Result import Result
from pyvrp.Statistics import Statistics
from pyvrp.diversity import different_neighbours

if TYPE_CHECKING:
    from pyvrp.ConvergenceManager import ConvergenceManager
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

    num_iter_no_improvement: int = 15_000

    def __post_init__(self):
        if self.num_iter_no_improvement < 0:
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
        conv_manager: ConvergenceManager,
        params: IteratedLocalSearchParams,
    ):
        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._perturb = perturb_method
        self._search = search_method
        self._accept = acceptance_criterion
        self._conv_manager = conv_manager
        self._params = params

    @property
    def _cost_evaluator(self) -> CostEvaluator:
        return self._pm.cost_evaluator()

    def _stats(self, solution: Solution) -> tuple[float, bool]:
        cost = self._cost_evaluator.penalised_cost(solution)
        is_feas = solution.is_feasible()
        return cost, is_feas

    def run(
        self,
        stop: StoppingCriterion,
        initial_solution: Solution,
        collect_stats: bool = True,
        display: bool = False,
        display_interval: float = 5.0,
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
        display
            Whether to display information about the solver progress. Default
            ``False``. Progress information is only available when
            ``collect_stats`` is also set, which it is by default.
        display_interval
            The interval (in seconds) at which to display progress information.

        Returns
        -------
        Result
            A Result object, containing statistics (if collected) and the best
            found solution.
        """
        print_progress = ProgressPrinter(
            should_print=display, display_interval=display_interval
        )
        print_progress.start(self._data)

        start = time.perf_counter()
        stats = Statistics(collect_stats=collect_stats)
        iters = 0
        iters_no_improvement = 0
        best = current = initial_solution

        while not stop(self._cost_evaluator.cost(best)):
            iters += 1

            if iters_no_improvement == self._params.num_iter_no_improvement:
                print_progress.restart()
                iters_no_improvement = 1
                current = best

            perturbed = self._perturb(
                current,
                self._pm.booster_cost_evaluator(),
                max(10, self._conv_manager.num_destroy),
            )
            diff = different_neighbours(current, perturbed)
            candidate = self._search(
                perturbed, self._pm.cost_evaluator(), diff
            )
            self._pm.register(candidate)

            if not candidate.is_feasible():
                candidate = self._search(
                    candidate, self._pm.booster_cost_evaluator(), diff
                )

                self._pm.register(candidate)

            curr_cost, curr_feas = self._stats(current)
            pert_cost, pert_feas = self._stats(perturbed)
            cand_cost, cand_feas = self._stats(candidate)
            best_cost, best_feas = self._stats(best)

            diff = len(different_neighbours(current, candidate))
            self._conv_manager.register(diff)

            if cand_feas and cand_cost < best_cost:  # new best
                best = candidate
                iters_no_improvement = 1
            else:
                iters_no_improvement += 1

            if cand_feas and self._accept(best_cost, curr_cost, cand_cost):
                current = candidate

            stats.collect(
                curr_cost,
                curr_feas,
                pert_cost,
                pert_feas,
                cand_cost,
                cand_feas,
                best_cost,
                best_feas,
                self._accept.threshold,  # type: ignore
                self._conv_manager.num_destroy,
                # diff,
            )
            print_progress.iteration(stats)

        runtime = time.perf_counter() - start
        res = Result(best, stats, iters, runtime)

        print_progress.end(res)

        return res

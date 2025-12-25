from __future__ import annotations

import time
from dataclasses import dataclass, field
from typing import TYPE_CHECKING

from pyvrp.ProgressPrinter import ProgressPrinter
from pyvrp.Result import Result
from pyvrp.Statistics import Statistics
from pyvrp._pyvrp import PopulationParams, SubPopulation
from pyvrp.diversity import broken_pairs_distance

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
    pop_params
        Population parameters. If not provided, a default will be used.
    """

    num_iters_no_improvement: int = 20_000
    pop_params: PopulationParams = field(
        default_factory=lambda: PopulationParams(
            min_pop_size=10, generation_size=0
        )
    )

    def __post_init__(self):
        if self.num_iters_no_improvement < 0:
            raise ValueError("num_iters_no_improvement < 0 not understood.")


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

        pop_params = self._params.pop_params
        pop = SubPopulation(broken_pairs_distance, pop_params)
        stats = Statistics(collect_stats=collect_stats)

        start = time.perf_counter()
        iters = iters_no_improvement = 0
        best = current = self._init

        cost_eval = self._pm.cost_evaluator()
        while not stop(cost_eval.cost(best)):
            iters += 1
            iters_no_improvement += 1

            if iters_no_improvement == self._params.num_iters_no_improvement:
                print_progress.restart()
                # Create a new population with increased minimum size + 1
                pop_params = PopulationParams(
                    min_pop_size=pop_params.min_pop_size + 1, generation_size=0
                )
                new = SubPopulation(broken_pairs_distance, pop_params)
                for sol in pop:
                    new.add(sol.solution, cost_eval)
                pop = new

                current = best
                iters_no_improvement = 0

            cost_eval = self._pm.cost_evaluator()
            candidate = self._search(current, cost_eval)
            self._pm.register(candidate)

            if cost_eval.cost(candidate) < cost_eval.cost(best):  # new best
                best = candidate
                iters_no_improvement = 0

            cand_cost = cost_eval.penalised_cost(candidate)

            # Accept candidate if its fitness is better than the worst
            # individual in the population.
            if len(pop) < pop_params.max_pop_size:
                pop.add(candidate, cost_eval)
                current = candidate
            else:
                worst_cost = max(
                    cost_eval.penalised_cost(item.solution) for item in pop
                )
                if cand_cost < worst_cost:
                    pop.add(candidate, cost_eval)
                    current = candidate

            stats.collect(current, candidate, best, cost_eval)
            print_progress.iteration(stats)

        runtime = time.perf_counter() - start
        res = Result(best, stats, iters, runtime)

        print_progress.end(res)

        return res

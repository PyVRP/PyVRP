from __future__ import annotations

import csv
import time
from dataclasses import dataclass
from importlib.metadata import version
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

# Templates for various different outputs.
_ITERATION = (
    "{special} {iters:>7} {elapsed:>6}s | {curr:>8} {cand:>8} {best:>8}"
)

_START = """PyVRP v{version}

Solving an instance with:
    {depot_text}
    {client_text}
    {vehicle_text} ({vehicle_type_text})

                  |   Cost (feasible)
    Iters    Time |   Curr    Cand     Best"""

_END = """
Search terminated in {runtime:.2f}s after {iters} iterations.
Best-found solution has cost {best_cost}.

{summary}
"""

NUM_ITERS_PRINT = 500


class ProgressPrinter:
    """
    A helper class that prints relevant solver progress information to the
    console, if desired.

    Parameters
    ----------
    should_print
        Whether to print information to the console. When ``False``, nothing is
        printed.
    """

    def __init__(self, should_print: bool):
        self._print = should_print
        self._best_cost = float("inf")

    def iteration(self, stats: Statistics):
        """
        Outputs relevant information every few hundred iterations. The output
        contains information about the feasible and infeasible populations,
        whether a new best solution has been found, and the search duration.
        """
        should_print = (
            self._print
            and stats.is_collecting()
            and stats.num_iterations % NUM_ITERS_PRINT == 0
        )

        if not should_print:
            return

        data = stats.data[-1]

        def format_cost(cost, is_feas):
            return f"{round(cost)} {'(F)' if is_feas else '(I)'}"

        msg = _ITERATION.format(
            special="H" if data.best_cost < self._best_cost else " ",
            iters=stats.num_iterations,
            elapsed=round(sum(stats.runtimes)),
            curr=format_cost(data.current_cost, data.current_feas),
            cand=format_cost(data.candidate_cost, data.candidate_feas),
            best=format_cost(data.best_cost, data.best_feas),
        )
        print(msg)

        if data.best_cost < self._best_cost:
            self._best_cost = data.best_cost

    def start(self, data: ProblemData):
        """
        Outputs information about PyVRP and the data instance that is being
        solved.
        """
        if not self._print:
            return

        num_d = data.num_depots
        depot_text = f"{num_d} depot{'s' if num_d > 1 else ''}"

        num_c = data.num_clients
        client_text = f"{num_c} client{'s' if num_c > 1 else ''}"

        num_v = data.num_vehicles
        vehicle_text = f"{num_v} vehicle{'s' if num_v > 1 else ''}"

        num_vt = data.num_vehicle_types
        vehicle_type_text = f"{num_vt} vehicle type{'s' if num_vt > 1 else ''}"

        msg = _START.format(
            version=version("pyvrp"),
            depot_text=depot_text,
            client_text=client_text,
            vehicle_text=vehicle_text,
            vehicle_type_text=vehicle_type_text,
        )
        print(msg)

    def end(self, result: Result):
        """
        Outputs information about the search duration and the best-found
        solution.
        """
        if self._print:
            msg = _END.format(
                iters=result.num_iterations,
                runtime=result.runtime,
                best_cost=round(result.cost(), 2),
                summary=result.summary(),
            )
            print(msg)


@dataclass
class _Datum:
    """
    Single ILS iteration data point.
    """

    current_cost: float
    current_feas: bool
    candidate_cost: float
    candidate_feas: bool
    best_cost: float
    best_feas: bool
    diversity: int


class Statistics:
    """
    Statistics about the Iterated Local Search progress.
    """

    def __init__(self, collect_stats: bool = True):
        self.runtimes: list[float] = []
        self.num_iterations = 0
        self.data: list[_Datum] = []

        self._clock = time.perf_counter()
        self._collect_stats = collect_stats

    def is_collecting(self) -> bool:
        return self._collect_stats

    def collect(
        self,
        current_cost: float,
        current_feas: bool,
        candidate_cost: float,
        candidate_feas: bool,
        best_cost: float,
        best_feas: bool,
        diversity: int,
    ):
        """
        Collects statistics from the ILS iteration.
        """
        if not self._collect_stats:
            return

        start = self._clock
        self._clock = time.perf_counter()

        self.runtimes.append(self._clock - start)
        self.num_iterations += 1

        datum = _Datum(
            current_cost,
            current_feas,
            candidate_cost,
            candidate_feas,
            best_cost,
            best_feas,
            diversity,
        )
        self.data.append(datum)

    def to_csv(self, file_path: str):
        """
        Exports the collected statistics to a CSV file.

        Parameters:
            file_path (str): The file path where the CSV will be saved.
        """
        if not self.data:
            print("No data to export.")
            return

        headers = [
            "current_cost",
            "current_feas",
            "candidate_cost",
            "candidate_feas",
            "best_cost",
            "best_feas",
            "diversity",
        ]

        with open(file_path, mode="w", newline="") as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(headers)
            for datum in self.data:
                writer.writerow(
                    [
                        datum.current_cost,
                        datum.current_feas,
                        datum.candidate_cost,
                        datum.candidate_feas,
                        datum.best_cost,
                        datum.best_feas,
                        datum.diversity,
                    ]
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
        print_progress = ProgressPrinter(should_print=display)
        print_progress.start(self._data)

        start = time.perf_counter()
        stats = Statistics(collect_stats=collect_stats)
        iters = 0
        best = current = initial_solution

        while not stop(self._cost_evaluator.cost(best)):
            iters += 1

            start = stop.criteria[0]._start_runtime
            max_runtime = stop.criteria[0]._max_runtime
            elapsed = (time.perf_counter() - start) / max_runtime
            num_destroy = int(10 + 15 * (1 - elapsed))

            perturbed = self._perturb(
                current, self._cost_evaluator, num_destroy
            )
            candidate_clients = self._diff(perturbed, current)
            candidate = self._search(
                perturbed,
                self._cost_evaluator,
                candidate_clients,
            )
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
                len(self._diff(candidate, current)),
            )
            print_progress.iteration(stats)

            if not candidate.is_feasible():
                continue  # skip infeasible solutiosn for now
            elif cand_cost < best_cost:
                best, current = candidate, candidate
            elif self._accept(best_cost, curr_cost, cand_cost):
                current = candidate

        runtime = time.perf_counter() - start
        res = Result(
            best,
            stats,
            iters,
            runtime,
        )

        print_progress.end(res)

        return res

    def _diff(self, new: Solution, old: Solution) -> list[int]:
        """
        Returns the clients that have changed between two solutions.
        """
        neighbours1 = new.neighbours()
        neighbours2 = old.neighbours()
        clients = range(self._data.num_depots, self._data.num_locations)
        return [idx for idx in clients if neighbours1[idx] != neighbours2[idx]]

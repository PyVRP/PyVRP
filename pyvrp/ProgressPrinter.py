from __future__ import annotations

from importlib.metadata import version
from time import perf_counter
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from pyvrp.Result import Result
    from pyvrp.Statistics import Statistics
    from pyvrp._pyvrp import (
        ProblemData,
    )

# Templates for various different outputs.
_ITERATION = "{special} {iters:>7} {elapsed:>6}s | {curr:>8} {pert:>8} {cand:>8} {best:>8}   | {threshold:>8} {diversity:>5}"  # noqa: E501

_START = """PyVRP v{version}

Solving an instance with:
    {depot_text}
    {client_text}
    {vehicle_text} ({vehicle_type_text})

                  |   Cost (feasible)
    Iters    Time |   Curr    Pert    Cand     Best       | Threshold (diff)  Diversity"""

_RESTART = "R                 |                restart                |        restart"

_END = """
Search terminated in {runtime:.2f}s after {iters} iterations.
Best-found solution has cost {best_cost}.

{summary}
"""


class ProgressPrinter:
    """
    A helper class that prints relevant solver progress information to the
    console, if desired.

    Parameters
    ----------
    should_print
        Whether to print information to the console. When ``False``, nothing is
        printed.
    display_interval
        Time (in seconds) between iteration logs.
    """

    def __init__(self, should_print: bool, display_interval: float):
        if display_interval < 0:
            raise ValueError("Expected display_interval >= 0.")

        self._print = should_print
        self._display_interval = display_interval
        self._last_print_time = perf_counter()
        self._best_cost = float("inf")

    def iteration(self, stats: Statistics):
        """
        Outputs relevant information every few hundred iterations. The output
        contains information about the feasible and infeasible populations,
        whether a new best solution has been found, and the search duration.
        """
        curr_time = perf_counter()
        interval = curr_time - self._last_print_time
        should_print = (
            self._print
            and stats.is_collecting()
            and interval >= self._display_interval
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
            pert=format_cost(data.perturbed_cost, data.perturbed_feas),
            cand=format_cost(data.candidate_cost, data.candidate_feas),
            best=format_cost(data.best_cost, data.best_feas),
            threshold=f"{data.threshold:.2f} ({data.threshold-data.current_cost:.2f})",  # noqa: E501
            diversity=round(data.diversity, 3),
        )
        print(msg)

        self._last_print_time = curr_time
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

    def restart(self):
        """
        Indicates in the progress information that the algorithm has restarted
        the search.
        """
        if self._print:
            print(_RESTART)

# This file is part of the PyVRP project (https://github.com/PyVRP/PyVRP), and
# licensed under the terms of the MIT license.

import logging
from importlib.metadata import version
from time import perf_counter

from pyvrp._pyvrp import ProblemData

from .Result import Result
from .Statistics import Statistics

logger = logging.getLogger(__name__)


# Templates for various different outputs.
_ITERATION = (
    "{special} {iters:>7} {elapsed:>6}s | "
    "{curr:>12}  {curr_feas} {cand:>12}  {cand_feas} {best:>12}  {best_feas}"
)

_START = """PyVRP v{version}

Solving an instance with:
    {depot_text}
    {client_text}
    {vehicle_text} ({vehicle_type_text})

    Iters    Time |      Current OK    Candidate OK         Best OK"""

_END = """
Search terminated in {runtime:.2f}s after {iters} iterations.
Best-found solution has cost {best_cost}.

{summary}
"""

_RESTART = "R                 |                      restart"


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
        Outputs relevant information every few seconds. The output contains
        information about the (penalised) cost and feasibility of the current,
        candidate, and best solutions, as well as the search duration.
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

        datum = stats.data[-1]
        new_best = datum.best_feas and datum.best_cost < self._best_cost
        msg = _ITERATION.format(
            special="H" if new_best else " ",
            iters=stats.num_iterations,
            elapsed=round(sum(stats.runtimes)),
            curr=datum.current_cost,
            curr_feas="Y" if datum.current_feas else "N",
            cand=datum.candidate_cost,
            cand_feas="Y" if datum.candidate_feas else "N",
            best=datum.best_cost,
            best_feas="Y" if datum.best_feas else "N",
        )

        logger.info(msg)

        self._last_print_time = curr_time
        if new_best:
            self._best_cost = datum.best_cost

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

        for line in msg.splitlines():
            logger.info(line)

    def end(self, result: Result):
        """
        Outputs information about the search duration and the best-found
        solution.
        """
        if self._print:
            msg = _END.format(
                iters=result.num_iterations,
                runtime=result.runtime,
                best_cost=result.cost(),
                summary=result.summary(),
            )

            for line in msg.splitlines():
                logger.info(line)

    def restart(self):
        """
        Indicates in the progress information that the algorithm has restarted
        the search.
        """
        if self._print:
            logger.info(_RESTART)

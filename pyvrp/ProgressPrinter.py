from importlib.metadata import version

from pyvrp._pyvrp import ProblemData

from .Result import Result
from .Statistics import Statistics

# Templates for various different outputs.
_ITERATION = (
    "{special} {iters:>7} {elapsed:>6}s | "
    "{feas_size:>3} {feas_avg:>8} {feas_best:>8} | "
    "{infeas_size:>3} {infeas_avg:>8} {infeas_best:>8}"
)

_START = """PyVRP v{version}

Solving an instance with:
    {num_depots} depots
    {num_clients} clients
    {num_vehicles} vehicles ({num_vehicle_types} types)

                  |       Feasible        |      Infeasible
    Iters    Time |   #      Avg     Best |   #      Avg     Best"""

_END = """
Search terminated in {runtime:.2f}s after {iters} iterations.
Best-found solution has cost {best_cost}.
"""

_RESTART = "R                 |        restart        |        restart"


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
        if not self._print or stats.num_iterations % 500 != 0:
            return

        feas = stats.feas_stats[-1]
        infeas = stats.infeas_stats[-1]

        msg = _ITERATION.format(
            special="H" if feas.best_cost < self._best_cost else " ",
            iters=stats.num_iterations,
            elapsed=round(sum(stats.runtimes)),
            feas_size=feas.size,
            feas_avg=round(feas.avg_cost),
            feas_best=round(feas.best_cost),
            infeas_size=infeas.size,
            infeas_avg=round(infeas.avg_cost),
            infeas_best=round(infeas.best_cost),
        )
        print(msg)

        if feas.best_cost < self._best_cost:
            self._best_cost = feas.best_cost

    def start(self, data: ProblemData):
        """
        Outputs information about PyVRP and the data instance that is being
        solved.
        """
        if self._print:
            msg = _START.format(
                version=version("pyvrp"),
                num_depots=data.num_depots,
                num_clients=data.num_clients,
                num_vehicles=data.num_vehicles,
                num_vehicle_types=data.num_vehicle_types,
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
            )
            print(msg)

    def restart(self):
        """
        Indicates in the progress information that the algorithm has restarted
        the search.
        """
        if self._print:
            print(_RESTART)

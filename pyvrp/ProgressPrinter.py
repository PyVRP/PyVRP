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
    - {num_depots} depots;
    - {num_clients} clients;
    - {num_vehicles} vehicles.

                  |       Feasible        |      Infeasible
    Iters    Time |   #      Avg     Best |   #      Avg     Best"""

_END = """
Search terminated in {runtime:.2f}s after {iters} iterations.
Best-found solution has cost {best_cost:.0f}.
"""

_RESTART = "R                 |        restart        |        restart"


class ProgressPrinter:
    """
    TODO
    """

    def __init__(self, should_print: bool):
        self._print = should_print
        self._best_cost = float("inf")

    def iteration(self, stats: Statistics):
        """
        TODO
        """
        if not self._print or stats.num_iterations % 500 != 0:
            return

        feas = stats.feas_stats[-1]
        infeas = stats.infeas_stats[-1]

        msg = _ITERATION.format(
            special="H" if feas.best_cost < self._best_cost else " ",
            iters=stats.num_iterations,
            elapsed=str(round(sum(stats.runtimes))),
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
        TODO
        """
        if self._print:
            msg = _START.format(
                version=version("pyvrp"),
                num_depots=data.num_depots,
                num_clients=data.num_clients,
                num_vehicles=data.num_vehicles,
            )
            print(msg)

    def end(self, result: Result):
        """
        TODO
        """
        if self._print:
            msg = _END.format(
                iters=result.num_iterations,
                runtime=result.runtime,
                best_cost=result.cost(),
            )
            print(msg)

    def restart(self):
        """
        TODO
        """
        if self._print:
            print(_RESTART)

from importlib.metadata import version

from pyvrp._pyvrp import ProblemData

from .Statistics import Statistics


class ProgressLogger:
    def __init__(self, log: bool):
        self._log = log

    def log_restart(self, iters_no_improvement: int):
        if self._log:
            print(
                f"{iters_no_improvement} iterations without improvement. "
                "Restarting the genetic algorithm.\n"
            )

    def log_progress(self, global_best: float, stats: Statistics):
        iters = stats.num_iterations

        if not self._log or iters % 500 != 0:
            return

        elapsed = round(sum(stats.runtimes), 2)
        msg = f"{iters:>7} {elapsed:>7} {global_best:>10} | "

        def _format_pop_stats(pop):
            size = pop.size
            avg_cost = round(pop.avg_cost, 2)
            best_cost = round(pop.best_cost, 2)
            return f"{size:>4} {avg_cost:>10} {best_cost:>10}"

        msg += _format_pop_stats(stats.feas_stats[-1]) + " | "
        msg += _format_pop_stats(stats.infeas_stats[-1])

        print(msg)

    def log_start(self, data: ProblemData):
        if not self._log:
            return

        msg = _centerize(f"PyVRP v{version('pyvrp')}") + "\n\n"

        msg += "Solving an instance with "
        msg += f"{_pluralize(data.num_depots, 'depot')}, "
        msg += f"{_pluralize(data.num_clients, 'client')}, "
        msg += f"and {_pluralize(data.num_vehicles, 'vehicle')}.\n\n"

        msg += _centerize("Solver progress") + "\n\n"
        msg += f"{'Statistics':^26} | {'Feasible':^26} | {'Infeasible':^26}\n"
        msg += f"{'Iters':>7} {'T (s)':>7} {'Best':>10} | "
        msg += f"{'Size':>4} {'Avg':>10} {'Best':>10} | "
        msg += f"{'Size':>4} {'Avg':>10} {'Best':>10}\n"

        print(msg)

    def log_end(self, iters: int, end: float, best_cost: float):
        if not self._log:
            return

        print("\n" + _centerize("Summary") + "\n")
        print(f"Search terminated after {iters} iterations and {end:.0f}s.")
        print(f"Best-found solution has cost {best_cost}.")
        print(_centerize("End"))


def _centerize(msg: str, length=82) -> str:
    padding = "-" * ((length - len(msg)) // 2)
    ending = "-" if (length - len(msg)) % 2 else ""
    return f"{padding} {msg} {padding}{ending}"


def _pluralize(num: int, word: str) -> str:
    return f"{num} {word}{'s' if num != 1 else ''}"

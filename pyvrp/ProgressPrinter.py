from importlib.metadata import version

from pyvrp._pyvrp import ProblemData

from .Statistics import Statistics


class ProgressPrinter:
    def __init__(self, should_print: bool):
        self._print = should_print

    def iteration(self, stats: Statistics):
        iters = stats.num_iterations
        elapsed = str(round(sum(stats.runtimes))) + "s"

        if not self._print or iters % 500 != 0:
            return

        def _format_pop_stats(pop):
            size = pop.size
            avg_cost = round(pop.avg_cost)
            best_cost = round(pop.best_cost)
            return f"{size:>4} {avg_cost:>8} {best_cost:>8}"

        print(
            f"{iters:>7} {elapsed:>7}",
            _format_pop_stats(stats.feas_stats[-1]),
            _format_pop_stats(stats.infeas_stats[-1]),
            sep=" | ",
        )

    def start(self, data: ProblemData):
        if not self._print:
            return

        print(f"PyVRP v{version('pyvrp')}")

        print()
        print(
            "Solving an instance with",
            f"{_pluralize(data.num_depots, 'depot')},",
            f"{_pluralize(data.num_clients, 'client')},",
            f"and {_pluralize(data.num_vehicles, 'vehicle')}.",
        )

        print()
        print(f"{'':^15} | {'Feasible':^22} | {'Infeasible':^22}")
        print(
            f"{'Iters':>7} {'Time':>7}",
            f"{'Size':>4} {'Avg':>8} {'Best':>8}",
            f"{'Size':>4} {'Avg':>8} {'Best':>8}",
            sep=" | ",
        )

    def end(self, iters: int, end: float, best_cost: float):
        if not self._print:
            return

        print(f"\nSearch terminated after {iters} iterations and {end:.0f}s.")
        print(f"Best-found solution has cost {best_cost}.")

    def restart(self, iters_no_improvement: int):
        if self._print:
            print(
                f"{iters_no_improvement} iterations without improvement. "
                "Restarting the genetic algorithm.\n"
            )


def _pluralize(num: int, word: str) -> str:
    return f"{num} {word}{'s' if num != 1 else ''}"

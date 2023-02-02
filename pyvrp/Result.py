from dataclasses import dataclass
from typing import Optional

import matplotlib.pyplot as plt

from pyvrp._lib.hgspy import Individual

from .Statistics import Statistics


class NotCollectedError(Exception):
    """
    Raised when statistics have not been collected, but a method is accessed
    that can only be used when statistics are available.
    """

    pass


@dataclass
class Result:
    """
    Stores the outcomes of a single run. An instance of this class is returned
    once the GeneticAlgorithm completes.

    Parameters
    ----------
    best
        The best observed solution.
    stats
        A Statistics object containing runtime statistics. These are only
        collected and available if statistics were collected for the given run.
    num_iterations
        Number of iterations performed by the genetic algorithm.
    runtime
        Total runtime of the main genetic algorithm loop.

    Raises
    ------
    ValueError
        When the number of iterations or runtime are negative.
    """

    best: Individual
    stats: Statistics
    num_iterations: int
    runtime: float

    def __post_init__(self):
        if self.num_iterations < 0:
            raise ValueError("Negative number of iterations not understood.")

        if self.runtime < 0:
            raise ValueError("Negative runtime not understood.")

    def cost(self) -> float:
        """
        Returns the cost (objective) value of the best solution.

        Returns
        -------
        float
            Objective value.
        """
        return self.best.cost()

    def is_feasible(self) -> bool:
        """
        Returns whether the best solution is feasible.

        Returns
        -------
        bool
            True when the solution is feasible, False otherwise.
        """
        return self.best.is_feasible()

    def has_statistics(self) -> bool:
        """
        Returns whether detailed statistics were collected. If statistics are
        not availabe, the plotting methods cannot be used.

        Returns
        -------
        bool
            True when detailed statistics are available, False otherwise.
        """
        return self.num_iterations == self.stats.num_iterations

    def plot(self, fig: Optional[plt.Figure] = None):
        """
        Plots the resulting instance, and detailed statistics about the
        algorithm's performance.

        Parameters
        ----------
        fig, optional
            Optional matplotlib Figure to draw on. One will be created if not
            provided.

        Raises
        ------
        NotCollectedError
            Raised when statistics have not been collected.
        """
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not fig:
            fig = plt.figure(figsize=(20, 12))

        gs = fig.add_gridspec(3, 2, width_ratios=(2 / 5, 3 / 5))

        self.plot_population(fig.add_subplot(gs[0, 0]))
        self.plot_objectives(fig.add_subplot(gs[1, 0]))
        self.plot_runtimes(fig.add_subplot(gs[2, 0]))
        self.plot_instance(fig.add_subplot(gs[:, 1]))

        plt.tight_layout()
        plt.draw_if_interactive()

    def plot_population(self, ax: Optional[plt.Axes] = None):
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not ax:
            _, ax = plt.subplots()

        ax.set_title("Population diversity")
        ax.set_xlabel("Iteration (#)")
        ax.set_ylabel("Avg. diversity")

        avg_feas_div = [d.avg_diversity for d in self.stats.feas_stats]
        ax.plot(
            range(self.num_iterations),
            avg_feas_div,
            label="Feas. diversity",
            c="tab:green",
        )

        avg_infeas_div = [d.avg_diversity for d in self.stats.feas_stats]
        ax.plot(
            range(self.num_iterations),
            avg_infeas_div,
            label="Infeas. diversity",
            c="tab:red",
        )

        ax.legend(frameon=False)

    def plot_objectives(self, ax: Optional[plt.Axes] = None):
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not ax:
            _, ax = plt.subplots()

    def plot_instance(self, ax: Optional[plt.Axes] = None):
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not ax:
            _, ax = plt.subplots()

    def plot_runtimes(self, ax: Optional[plt.Axes] = None):
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not ax:
            _, ax = plt.subplots()

    def __str__(self) -> str:
        summary = [
            "Solution results",
            "================",
            f"   objective: {self.cost():.2f}",
            f"    feasible? {self.is_feasible()}",
            f"# iterations: {self.num_iterations}",
            f"    run-time: {self.runtime:.2f} seconds\n",
        ]

        routes = [
            "Routes",
            "======",
        ]

        for idx, route in enumerate(self.best.get_routes()):
            if route:
                routes.append(f"Route {idx:>2}: {route}")

        return "\n".join(summary + routes)

from dataclasses import dataclass
from typing import Optional, Tuple

import matplotlib.pyplot as plt
import numpy as np

from pyvrp._lib.hgspy import Individual, ProblemData

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

    def plot(self, data: ProblemData, fig: Optional[plt.Figure] = None):
        """
        Plots the resulting instance, and detailed statistics about the
        algorithm's performance.

        Parameters
        ----------
        data
            Data instance underlying this result's solution.
        fig, optional
            Optional Figure to draw on. One will be created if not provided.

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
        self.plot_solution(data, fig.add_subplot(gs[:, 1]))

        plt.tight_layout()

    def plot_population(self, ax: Optional[plt.Axes] = None):
        """
        Plots population diversity statistics.

        Parameters
        ----------
        ax, optional
            Axes object to draw the plot on. One will be created if not
            provided.

        Raises
        ------
        NotCollectedError
            Raised when statistics have not been collected.
        """
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not ax:
            _, ax = plt.subplots()

        ax.set_title("Population diversity")
        ax.set_xlabel("Iteration (#)")
        ax.set_ylabel("Avg. diversity")

        x = 1 + np.arange(self.num_iterations)
        y = [d.avg_diversity for d in self.stats.feas_stats]
        ax.plot(x, y, label="Feas. diversity", c="tab:green")

        y = [d.avg_diversity for d in self.stats.feas_stats]
        ax.plot(x, y, label="Infeas. diversity", c="tab:red")

        ax.legend(frameon=False)

    def plot_objectives(
        self,
        ylim_adjust: Tuple[float, float] = (0.995, 1.03),
        ax: Optional[plt.Axes] = None,
    ):
        """
        Plots population objective values.

        Parameters
        ----------
        ylim_adjust, optional
            A tuple of ylim adjustment fractions. The two values are multiplied
            by the best solution cost to provide ylim bounds. These bounds are
            passed to ax.ylim().
        ax, optional
            Axes object to draw the plot on. One will be created if not
            provided.

        Raises
        ------
        NotCollectedError
            Raised when statistics have not been collected.
        """
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not ax:
            _, ax = plt.subplots()

        x = 1 + np.arange(self.num_iterations)
        y = np.minimum.accumulate([d.best_cost for d in self.stats.feas_stats])
        ax.plot(x, y, label="Global feas best", c="tab:blue")

        y = [d.best_cost for d in self.stats.feas_stats]
        ax.plot(x, y, label="Feas best", c="tab:green")

        y = [d.avg_cost for d in self.stats.feas_stats]
        ax.plot(x, y, label="Feas avg.", c="tab:green", alpha=0.3, ls="dashed")

        y = [d.best_cost for d in self.stats.infeas_stats]
        ax.plot(x, y, label="Infeas best", c="tab:red")

        y = [d.avg_cost for d in self.stats.infeas_stats]
        ax.plot(x, y, label="Infeas avg.", c="tab:red", alpha=0.3, ls="dashed")

        ax.set_title("Population objectives")
        ax.set_xlabel("Iteration (#)")
        ax.set_ylabel("Objective")

        # We're really only interested in the algorithm's performance 'near'
        # the eventual best solution.
        ax.set_ylim(ylim_adjust[0] * self.cost(), ylim_adjust[1] * self.cost())
        ax.legend(frameon=False)

    def plot_solution(self, data: ProblemData, ax: Optional[plt.Axes] = None):
        """
        Plots best solution.

        Parameters
        ----------
        data
            Data instance underlying this result's solution.
        ax, optional
            Axes object to draw the plot on. One will be created if not
            provided.

        Raises
        ------
        NotCollectedError
            Raised when statistics have not been collected.
        """
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not ax:
            _, ax = plt.subplots()

        coords = [
            (data.client(client).x, data.client(client).y)
            for client in range(data.num_clients())
        ]

        kwargs = dict(zorder=3, s=100)
        ax.scatter(*coords, c="tab:blue", label="Customers", **kwargs)

        kwargs = dict(marker="*", zorder=3, s=750)  # type: ignore
        ax.scatter(*coords[0], c="tab:red", label="Depot", **kwargs)

        for route in self.best.get_routes():
            ax.plot(*coords[[0] + route + [0]].T)

        ax.grid(color="grey", linestyle="--", linewidth=0.25)

        ax.set_title("Solution")
        ax.set_aspect("equal", "datalim")
        ax.legend(frameon=False, ncol=2)

    def plot_runtimes(self, ax: Optional[plt.Axes] = None):
        """
        Plots iteration runtimes.

        Parameters
        ----------
        ax, optional
            Axes object to draw the plot on. One will be created if not
            provided.

        Raises
        ------
        NotCollectedError
            Raised when statistics have not been collected.
        """
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not ax:
            _, ax = plt.subplots()

        x = 1 + np.arange(self.num_iterations)
        ax.plot(x, self.stats.runtimes)

        if self.num_iterations > 1:  # need data to plot a trendline
            b, c = np.polyfit(x, self.stats.runtimes, 1)  # noqa
            ax.plot(b * x + c)

        ax.set_xlim(left=0)

        ax.set_xlabel("Iteration (#)")
        ax.set_ylabel("Run-time (s)")
        ax.set_title("Run-time per iteration")

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

        for idx, route in enumerate(self.best.get_routes(), 1):
            if route:
                routes.append(f"Route {idx:>2}: {route}")

        return "\n".join(summary + routes)

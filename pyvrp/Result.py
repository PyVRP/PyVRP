from dataclasses import dataclass
from typing import Optional

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
        return (
            self.num_iterations > 0
            and self.num_iterations == self.stats.num_iterations
        )

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

        # Uses a GridSpec instance to lay-out all subplots nicely. There are
        # two columns: left and right. Left has three rows, each containing a
        # plot with statistics: the first plots population diversity, the
        # second subpopulation objective information, and the third iteration
        # runtimes. The right column plots the solution on top of the instance
        # data.
        gs = fig.add_gridspec(3, 2, width_ratios=(2 / 5, 3 / 5))

        ax_diversity = fig.add_subplot(gs[0, 0])
        self.plot_diversity(ax=ax_diversity)
        self.plot_objectives(ax=fig.add_subplot(gs[1, 0], sharex=ax_diversity))
        self.plot_runtimes(ax=fig.add_subplot(gs[2, 0], sharex=ax_diversity))
        self.plot_solution(data, ax=fig.add_subplot(gs[:, 1]))

        plt.tight_layout()

    def plot_diversity(self, ax: Optional[plt.Axes] = None):
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

        ax.set_title("Diversity")
        ax.set_xlabel("Iteration (#)")
        ax.set_ylabel("Avg. diversity")

        x = 1 + np.arange(self.num_iterations)
        y = [d.avg_diversity for d in self.stats.feas_stats]
        ax.plot(x, y, label="Feas. diversity", c="tab:green")

        y = [d.avg_diversity for d in self.stats.infeas_stats]
        ax.plot(x, y, label="Infeas. diversity", c="tab:red")

        ax.legend(frameon=False)

    def plot_objectives(
        self, num_to_skip: Optional[int] = None, ax: Optional[plt.Axes] = None
    ):
        """
        Plots each subpopulation's objective values.

        Parameters
        ----------
        num_to_skip, optional
            Number of initial iterations to skip in the plot. These early
            iterations typically have very high objective values, and obscure
            what's going on later in the search. The default skips the first 5%
            of iterations.
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

        if num_to_skip is None:
            num_to_skip = int(0.05 * self.num_iterations)

        def _plot(x, y, *args, **kwargs):
            ax.plot(x[num_to_skip:], y[num_to_skip:], *args, **kwargs)

        x = 1 + np.arange(self.num_iterations)
        y = [d.best_cost for d in self.stats.feas_stats]
        _plot(x, y, label="Feas. best", c="tab:green")

        y = [d.avg_cost for d in self.stats.feas_stats]
        _plot(x, y, label="Feas. avg.", c="tab:green", alpha=0.3, ls="--")

        y = [d.best_cost for d in self.stats.infeas_stats]
        _plot(x, y, label="Infeas. best", c="tab:red")

        y = [d.avg_cost for d in self.stats.infeas_stats]
        _plot(x, y, label="Infeas. avg.", c="tab:red", alpha=0.3, ls="--")

        ax.set_title("Feasible objectives")
        ax.set_xlabel("Iteration (#)")
        ax.set_ylabel("Objective")

        ax.legend(frameon=False)

    def plot_solution(self, data: ProblemData, ax: Optional[plt.Axes] = None):
        """
        Plots the best solution.

        Parameters
        ----------
        data
            Data instance underlying this result's solution.
        ax, optional
            Axes object to draw the plot on. One will be created if not
            provided.
        """
        if not ax:
            _, ax = plt.subplots()

        dim = data.num_clients() + 1
        x_coords = np.array([data.client(client).x for client in range(dim)])
        y_coords = np.array([data.client(client).y for client in range(dim)])

        # This is the depot
        kwargs = dict(c="tab:red", marker="*", zorder=3, s=500)
        ax.scatter(x_coords[0], y_coords[0], label="Depot", **kwargs)

        for idx, route in enumerate(self.best.get_routes(), 1):
            if route:
                x = x_coords[route]
                y = y_coords[route]

                # Coordinates of customers served by this route.
                ax.scatter(x, y, label=f"Route {idx}", zorder=3, s=75)
                ax.plot(x, y)

                kwargs = dict(ls=(0, (5, 15)), linewidth=0.25, color="grey")
                ax.plot([x_coords[0], x[0]], [y_coords[0], y[1]], **kwargs)
                ax.plot([x[-1], x_coords[0]], [y[-1], y_coords[0]], **kwargs)

        ax.grid(color="grey", linestyle="solid", linewidth=0.2)

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
        ax.set_ylabel("Runtime (s)")
        ax.set_title("Iteration runtimes")

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

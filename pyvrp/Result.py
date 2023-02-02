from dataclasses import dataclass
from random import shuffle
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

        # Uses a GridSpec instance to lay-out all subplots nicely. There are
        # two columns: left and right. Left has four rows, each containg a
        # plot with statistics: the first plots population diversity, the
        # second feasible subpopulation objective information, the third
        # infeasible subpopulation objective information, and the fourth
        # iteration runtimes. The right column plots the solution on top of the
        # instance data.
        gs = fig.add_gridspec(4, 2, width_ratios=(2 / 5, 3 / 5))

        ax_diversity = fig.add_subplot(gs[0, 0])
        self.plot_diversity(ax=ax_diversity)

        ax_feas = fig.add_subplot(gs[1, 0], sharex=ax_diversity)
        self.plot_feasible_objectives(ax=ax_feas)

        ax_infeas = fig.add_subplot(gs[2, 0], sharex=ax_diversity)
        self.plot_infeasible_objectives(ax=ax_infeas)

        self.plot_runtimes(ax=fig.add_subplot(gs[3, 0], sharex=ax_diversity))

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

    def plot_feasible_objectives(self, ax: Optional[plt.Axes] = None):
        """
        Plots feasible subpopulation objective values.

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
        y = [d.best_cost for d in self.stats.feas_stats]
        ax.plot(x, y, label="Feas. best", c="tab:green")

        y = [d.avg_cost for d in self.stats.feas_stats]
        ax.plot(x, y, label="Feas. avg.", c="tab:green", alpha=0.3, ls="--")

        ax.set_title("Feasible objectives")
        ax.set_xlabel("Iteration (#)")
        ax.set_ylabel("Objective")

        ax.legend(frameon=False)

    def plot_infeasible_objectives(self, ax: Optional[plt.Axes] = None):
        """
        Plots feasible subpopulation objective values.

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
        y = [d.best_cost for d in self.stats.infeas_stats]
        ax.plot(x, y, label="Infeas. best", c="tab:red")

        y = [d.avg_cost for d in self.stats.infeas_stats]
        ax.plot(x, y, label="Infeas. avg.", c="tab:red", alpha=0.3, ls="--")

        ax.set_title("Infeasible objectives")
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

        Raises
        ------
        NotCollectedError
            Raised when statistics have not been collected.
        """
        if not self.has_statistics():
            raise NotCollectedError("Statistics have not been collected.")

        if not ax:
            _, ax = plt.subplots()

        dim = data.num_clients() + 1
        x_coords = np.array([data.client(client).x for client in range(dim)])
        y_coords = np.array([data.client(client).y for client in range(dim)])

        # This is the depot
        kwargs = dict(c="tab:red", marker="*", zorder=3, s=500)
        ax.scatter(x_coords[0], y_coords[0], label="Depot", **kwargs)

        routes = self.best.get_routes()
        num_routes = self.best.num_routes()

        # Since we're using ax.quiver, we need to provide colours explicitly.
        cmap = plt.cm.get_cmap("tab20c")
        colours = [cmap(c) for c in np.linspace(0.0, 1.0, num=num_routes)]
        shuffle(colours)  # so close routes will likely get dissimilar colours

        for colour, (idx, route) in zip(colours, enumerate(routes, 1)):
            if route:
                x = x_coords[route]
                x_dir = -np.diff(x, prepend=x[0])

                y = y_coords[route]
                y_dir = -np.diff(y, prepend=y[0])

                # Coordinates of customers served by this route.
                kwargs = dict(zorder=3, s=50)
                ax.scatter(x, y, color=colour, label=f"Route {idx}")

                kwargs = dict(scale=1, scale_units="xy", units="dots", width=4)
                ax.quiver(x, y, x_dir, y_dir, color=colour, **kwargs)

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

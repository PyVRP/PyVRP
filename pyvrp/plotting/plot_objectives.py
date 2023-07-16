from typing import Optional, Tuple

import matplotlib.pyplot as plt
import numpy as np

from pyvrp.Result import Result


def plot_objectives(
    result: Result,
    num_to_skip: Optional[int] = None,
    ax: Optional[plt.Axes] = None,
    ylim_adjust: Tuple[float, float] = (0.95, 1.15),
):
    """
    Plots each subpopulation's objective values.

    Parameters
    ----------
    result
        Result for which to plot objectives.
    num_to_skip
        Number of initial iterations to skip when plotting. Early iterations
        often have very high objective values, and obscure what's going on
        later in the search. The default skips the first 5% of iterations.
    ax
        Axes object to draw the plot on. One will be created if not provided.
    ylim_adjust
        Bounds the y-axis to ``(best * ylim_adjust[0], best * ylim_adjust[1])``
        where ``best`` denotes the best found feasible objective value. Default
        (0.95, 1.15).
    """
    if not ax:
        _, ax = plt.subplots()

    if num_to_skip is None:
        num_to_skip = int(0.05 * result.num_iterations)

    def _plot(x, y, *args, **kwargs):
        ax.plot(x[num_to_skip:], y[num_to_skip:], *args, **kwargs)

    x = 1 + np.arange(result.num_iterations)
    y = [d.best_cost for d in result.stats.infeas_stats]
    _plot(x, y, label="Infeas. best", c="tab:red")

    y = [d.avg_cost for d in result.stats.infeas_stats]
    _plot(x, y, label="Infeas. avg.", c="tab:red", alpha=0.3, ls="--")

    y = [d.best_cost for d in result.stats.feas_stats]
    _plot(x, y, label="Feas. best", c="tab:green")

    y = [d.avg_cost for d in result.stats.feas_stats]
    _plot(x, y, label="Feas. avg.", c="tab:green", alpha=0.3, ls="--")

    # Use best objectives to set reasonable y-limits
    best = np.nanmin([d.best_cost for d in result.stats.feas_stats])
    ax.set_ylim(best * ylim_adjust[0], best * ylim_adjust[1])

    ax.set_title("Objectives")
    ax.set_xlabel("Iteration (#)")
    ax.set_ylabel("Objective")

    ax.legend(frameon=False)

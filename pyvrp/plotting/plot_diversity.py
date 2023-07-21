from typing import Optional

import matplotlib.pyplot as plt
import numpy as np

from pyvrp.Result import Result


def plot_diversity(result: Result, ax: Optional[plt.Axes] = None):
    """
    Plots population diversity statistics.

    Parameters
    ----------
    result
        Result for which to plot diversity.
    ax, optional
        Axes object to draw the plot on. One will be created if not
        provided.
    """
    if not ax:
        _, ax = plt.subplots()

    ax.set_title("Diversity")
    ax.set_xlabel("Iteration (#)")
    ax.set_ylabel("Avg. diversity")

    x = 1 + np.arange(result.num_iterations)
    y = [d.avg_diversity for d in result.stats.feas_stats]
    ax.plot(x, y, label="Feas. diversity", c="tab:green")

    y = [d.avg_diversity for d in result.stats.infeas_stats]
    ax.plot(x, y, label="Infeas. diversity", c="tab:red")

    ax.legend(frameon=False)

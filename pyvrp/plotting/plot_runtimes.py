from typing import Optional

import matplotlib.pyplot as plt
import numpy as np

from pyvrp.Result import Result


def plot_runtimes(result: Result, ax: Optional[plt.Axes] = None):
    """
    Plots iteration runtimes.

    Parameters
    ----------
    result
        Result for which to plot runtimes.
    ax, optional
        Axes object to draw the plot on. One will be created if not provided.
    """
    if not ax:
        _, ax = plt.subplots()

    x = 1 + np.arange(result.num_iterations)
    ax.plot(x, result.stats.runtimes)

    if result.num_iterations > 1:  # need data to plot a trendline
        b, c = np.polyfit(x, result.stats.runtimes, 1)
        ax.plot(b * x + c)

    ax.set_xlim(left=0)

    ax.set_xlabel("Iteration (#)")
    ax.set_ylabel("Runtime (s)")
    ax.set_title("Iteration runtimes")

from typing import Optional

import matplotlib.pyplot as plt

from pyvrp import ProblemData
from pyvrp.Result import Result
from pyvrp.exceptions import StatisticsNotCollectedError

from .plot_diversity import plot_diversity
from .plot_objectives import plot_objectives
from .plot_runtimes import plot_runtimes
from .plot_solution import plot_solution


def plot_result(
    result: Result, data: ProblemData, fig: Optional[plt.Figure] = None
):
    """
    Plots the results of a run: the best solution, and detailed
    statistics about the algorithm's performance.

    Parameters
    ----------
    result
        Result to be plotted.
    data
        Data instance underlying the result's solution.
    fig, optional
        Optional Figure to draw on. One will be created if not provided.

    Raises
    ------
    StatisticsNotCollectedError
        Raised when statistics have not been collected.
    """
    if not result.has_statistics():
        raise StatisticsNotCollectedError(
            "Statistics have not been collected."
        )

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
    plot_diversity(result, ax=ax_diversity)
    plot_objectives(result, ax=fig.add_subplot(gs[1, 0], sharex=ax_diversity))
    plot_runtimes(result, ax=fig.add_subplot(gs[2, 0], sharex=ax_diversity))
    plot_solution(result.best, data, ax=fig.add_subplot(gs[:, 1]))

    plt.tight_layout()

# This file is part of the PyVRP project (https://github.com/PyVRP/PyVRP),
# licensed under the terms of the MIT license.

import matplotlib.pyplot as plt

from pyvrp import ProblemData
from pyvrp.Result import Result
from pyvrp.plotting.plot_objectives import plot_objectives
from pyvrp.plotting.plot_runtimes import plot_runtimes
from pyvrp.plotting.plot_solution import plot_solution


def plot_result(
    result: Result, data: ProblemData, fig: plt.Figure | None = None
):
    """
    Plots the results of a run, including the best solution and detailed
    statistics about the algorithm's performance.

    Parameters
    ----------
    result
        Result to be plotted.
    data
        Data instance underlying the result's solution.
    fig
        Optional Figure to draw on. One will be created if not provided.
    """
    if not fig:
        fig = plt.figure(figsize=(20, 12))

    # Uses a GridSpec instance to lay-out all subplots nicely. There are
    # two columns: left and right. Left has two rows, each containing a
    # plot with statistics: the first plots each iteration's objective
    # information, and the second plots iteration runtimes. The right column
    # plots the solution on top of the instance data.
    gs = fig.add_gridspec(2, 2, width_ratios=(2 / 5, 3 / 5))

    ax_div = fig.add_subplot(gs[0, 0])
    plot_objectives(result, ax=ax_div)
    plot_runtimes(result, ax=fig.add_subplot(gs[1, 0], sharex=ax_div))

    plot_solution(result.best, data, ax=fig.add_subplot(gs[:, 1]))

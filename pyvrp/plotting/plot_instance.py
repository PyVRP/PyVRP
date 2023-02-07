from typing import Optional

import matplotlib.pyplot as plt

from pyvrp import ProblemData

from .plot_coordinates import plot_coordinates
from .plot_demands import plot_demands
from .plot_timewindows import plot_timewindows


def plot_instance(data: ProblemData, fig: Optional[plt.Figure] = None):
    """
    Plots coordinates, time windows and demands for an instance.

    Parameters
    ----------
    data
        Data instance for which to plot coordinates, time windows and demands.
    fig, optional
        Optional Figure to draw on. One will be created if not provided.
    """
    if not fig:
        fig = plt.figure(figsize=(20, 12))

    # Uses a GridSpec instance to lay-out all subplots nicely. There are
    # two columns: left and right. Left has two rows: the first plots time
    # windows and the second plots demands. The right column plots the
    # client and depot locations on a grid.
    gs = fig.add_gridspec(2, 2, width_ratios=(2 / 5, 3 / 5))

    plot_timewindows(data, ax=fig.add_subplot(gs[0, 0]))
    plot_demands(data, ax=fig.add_subplot(gs[1, 0]))
    plot_coordinates(data, ax=fig.add_subplot(gs[:, 1]))

    plt.tight_layout()

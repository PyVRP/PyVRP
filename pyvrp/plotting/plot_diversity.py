import matplotlib.pyplot as plt

from pyvrp.Result import Result


def plot_diversity(result: Result, ax: plt.Axes | None = None):
    """
    Plots population diversity statistics.

    Parameters
    ----------
    result
        Result for which to plot diversity.
    ax
        Axes object to draw the plot on. One will be created if not
        provided.
    """
    if not ax:
        _, ax = plt.subplots()

    ax.set_title("Diversity")
    ax.set_xlabel("Iteration (#)")
    ax.set_ylabel("Avg. diversity")

    # TODO update later

    ax.legend(frameon=False)

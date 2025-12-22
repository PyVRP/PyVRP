import matplotlib.pyplot as plt
import numpy as np

from pyvrp.Result import Result


def plot_objectives(
    result: Result,
    num_to_skip: int | None = None,
    ax: plt.Axes | None = None,
    ylim_adjust: tuple[float, float] = (0.99, 1.05),
):
    """
    Plots each iteration's objective values.

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
        Optional adjustment to bound the y-axis to ``(best * ylim_adjust[0],
        best * ylim_adjust[1])`` where ``best`` denotes the best found feasible
        objective value.
    """
    if not ax:
        _, ax = plt.subplots()

    if num_to_skip is None:
        num_to_skip = int(0.05 * result.num_iterations)

    def _plot(x, y, *args, **kwargs):
        ax.plot(x[num_to_skip:], y[num_to_skip:], *args, **kwargs)

    x = 1 + np.arange(result.num_iterations)

    y = [datum.current_cost for datum in result.stats]
    _plot(x, y, label="Current")

    y = [datum.candidate_cost for datum in result.stats]
    _plot(x, y, label="Candidate", alpha=0.2, zorder=1)

    y = [datum.best_cost for datum in result.stats]
    _plot(x, y, label="Best")

    # Use best-found solution to set reasonable y-limits, if available.
    if result.is_feasible():
        best_cost = result.cost()
        ax.set_ylim(best_cost * ylim_adjust[0], best_cost * ylim_adjust[1])

    ax.set_title("Objectives")
    ax.set_xlabel("Iteration (#)")
    ax.set_ylabel("Objective")

    ax.legend(frameon=False)

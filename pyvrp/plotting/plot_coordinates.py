import matplotlib.pyplot as plt
import numpy as np

from pyvrp import ProblemData


def plot_coordinates(
    data: ProblemData,
    title: str = "Coordinates",
    ax: plt.Axes | None = None,
):
    """
    Plots coordinates for clients and depot.

    Parameters
    ----------
    data
        Data instance.
    title
        Title to add to the plot.
    ax
        Axes object to draw the plot on. One will be created if not provided.
    """
    if not ax:
        _, ax = plt.subplots()

    num_locs = data.num_locations
    x_coords = np.empty((num_locs,))
    y_coords = np.empty((num_locs,))
    for idx, depot in enumerate(data.depots()):
        x_coords[idx] = depot.x
        y_coords[idx] = depot.y
    for idx, client in enumerate(data.clients(), data.num_depots):
        x_coords[idx] = client.x
        y_coords[idx] = client.y

    # These are the depots
    kwargs = dict(c="tab:red", marker="*", zorder=3, s=500)
    ax.scatter(
        x_coords[: data.num_depots],
        y_coords[: data.num_depots],
        label="Depot",
        **kwargs,
    )

    ax.scatter(
        x_coords[data.num_depots :],
        y_coords[data.num_depots :],
        s=50,
        label="Clients",
    )

    ax.grid(color="grey", linestyle="solid", linewidth=0.2)

    ax.set_title(title)
    ax.set_aspect("equal", "datalim")
    ax.legend(frameon=False, ncol=2)

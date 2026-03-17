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

    x_coords = np.array([loc.x for loc in data.locations()])
    y_coords = np.array([loc.y for loc in data.locations()])

    # These are the depots
    kwargs = dict(c="tab:red", marker="*", zorder=3, s=500)
    depot_locs = [depot.location for depot in data.depots()]
    ax.scatter(
        x_coords[depot_locs],
        y_coords[depot_locs],
        label="Depot",
        **kwargs,
    )

    client_locs = [client.location for client in data.clients()]
    ax.scatter(
        x_coords[client_locs],
        y_coords[client_locs],
        s=50,
        label="Clients",
    )

    ax.grid(color="grey", linestyle="solid", linewidth=0.2)

    ax.set_title(title)
    ax.set_aspect("equal", "datalim")
    ax.legend(frameon=False, ncol=2)

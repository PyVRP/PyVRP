from typing import Optional

import matplotlib.pyplot as plt
import numpy as np

from pyvrp import ProblemData


def plot_demands(
    data: ProblemData,
    title: Optional[str] = None,
    ax: Optional[plt.Axes] = None,
):
    """
    Plots demands for clients, as vertical bars sorted by demand.

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

    demand = np.array([client.demand for client in data.clients()])
    demand = np.sort(demand)

    ax.bar(np.arange(data.num_depots, data.dimension), demand)

    if title is None:
        num_types = data.num_vehicle_types
        weights = [
            data.vehicle_type(idx).num_available for idx in range(num_types)
        ]
        capacities = [
            data.vehicle_type(idx).capacity for idx in range(num_types)
        ]
        mean_capacity = np.average(capacities, weights=weights)
        title = (
            f"Demands (avg. cap = {mean_capacity:.2f}, "
            + f"{mean_capacity / demand.mean():.2f} stops/route)"
        )

    ax.set_title(title)
    ax.set_xlabel("Client (sorted by demand)")
    ax.set_ylabel("Demand")

import matplotlib.pyplot as plt
import numpy as np

from pyvrp import ProblemData


def plot_demands(
    data: ProblemData,
    title: str | None = None,
    ax: plt.Axes | None = None,
    dimension: int = 0,
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
    dimension
        Load dimension to plot. Default 0.
    """
    if not ax:
        _, ax = plt.subplots()

    clients = data.clients()
    demand = np.array([client.delivery[dimension] for client in clients])
    demand = np.sort(demand)

    ax.bar(np.arange(data.num_depots, data.num_locations), demand)

    if title is None:
        veh_types = data.vehicle_types()
        weights = [veh_type.num_available for veh_type in veh_types]
        capacities = [veh_type.capacity[dimension] for veh_type in veh_types]
        mean_capacity = np.average(capacities, weights=weights)

        title = (
            f"Demands (avg. cap = {mean_capacity:.2f}, "
            f"{mean_capacity / demand.mean():.2f} stops/route)"
        )

    ax.set_title(title)
    ax.set_xlabel("Client (sorted by demand)")
    ax.set_ylabel("Demand")

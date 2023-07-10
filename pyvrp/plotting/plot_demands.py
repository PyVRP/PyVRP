from typing import Optional

import matplotlib.pyplot as plt
import numpy as np

from pyvrp import ProblemData

def plot_demands(
    data: ProblemData,
    demand_type: str,
    title: Optional[str] = None,
    ax: Optional[plt.Axes] = None,
):
    """
    Plots demands for clients, as vertical bars sorted by demand.

    Parameters
    ----------
    data
        Data instance.
    title, optional
        Title to add to the plot.
    ax, optional
        Axes object to draw the plot on. One will be created if not provided.
    demand_type
        String specifying the type of demand to plot. 'weight', 'volume' or 'salvage.
    """
    dim = data.num_clients + 1
    if demand_type == 'weight':
        demands = np.array([data.client(client).demandWeight for client in range(1, dim)])
    elif demand_type == 'volume':
        demands = np.array([data.client(client).demandVolume for client in range(1, dim)])
    elif demand_type == 'salvage':
        demands = np.array([data.client(client).demandSalvage for client in range(1, dim)])
    else:
        raise ValueError("Invalid demand type. Choose 'weight', 'volume' or 'salvage'.")

    if demand_type != 'salvage':
        demands = np.sort(demands)

    if not ax:
        fig, ax = plt.subplots()
        
    ax.bar(np.arange(1, dim), demands, color='blue')
    ax.set_title(title)
    ax.set_xlabel("Client (sorted by demand)")
    ax.set_ylabel("Demand")

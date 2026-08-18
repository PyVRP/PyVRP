from itertools import pairwise
from typing import Sequence

import matplotlib.pyplot as plt
import numpy as np

from pyvrp import Activity, ActivityType, ProblemData, Solution


def plot_solution(
    solution: Solution,
    data: ProblemData,
    plot_locations: bool = False,
    ax: plt.Axes | None = None,
):
    """
    Plots the given solution.

    Parameters
    ----------
    solution
        Solution to plot.
    data
        Data instance underlying the solution.
    plot_locations
        Whether to plot all locations as dots. Default False, which plots only
        the solution's routes.
    ax
        Axes object to draw the plot on. One will be created if not provided.
    """
    if not ax:
        _, ax = plt.subplots()

    x_coords = np.array([loc.x for loc in data.locations()])
    y_coords = np.array([loc.y for loc in data.locations()])

    # These are the depots, as big red stars.
    kwargs = dict(label="Depot", c="tab:red", marker="*", zorder=3, s=500)
    depot_locs = np.array([depot.location for depot in data.depots()], int)
    ax.scatter(x_coords[depot_locs], y_coords[depot_locs], **kwargs)

    colors = plt.get_cmap("tab10")
    for idx, route in enumerate(solution.routes()):
        color = colors(idx % colors.N)

        activities: Sequence[Activity] = route.schedule()
        depots = [act.idx for act in activities if act.is_depot()]

        trips: list[list[int]] = []  # as lists of client locations
        for activity in activities[:-1]:
            match activity.type:
                case ActivityType.DEPOT:
                    trips.append([])

                case ActivityType.CLIENT:
                    client = data.client(activity.idx)
                    trips[-1].append(client.location)

                case ActivityType.PICKUP:
                    pickup = data.shipment(activity.idx).pickup
                    trips[-1].append(pickup.location)

                case ActivityType.DELIVERY:
                    delivery = data.shipment(activity.idx).delivery
                    trips[-1].append(delivery.location)

                case _:
                    continue

        for locations, (start, end) in zip(trips, pairwise(depots)):
            if len(locations) == 0:
                continue

            x = x_coords[locations]
            y = y_coords[locations]

            # Clients visited by this trip, as a line segment or single dot (in
            # case of a singleton trip). Trips of the same route share colour.
            if plot_locations or len(locations) == 1:
                kwargs = dict(label=f"Route {idx + 1}", zorder=3, s=75)
                ax.scatter(x, y, **kwargs, color=color)

            if len(locations) > 1:
                ax.plot(x, y, color=color)

            # Thin edges from and to the depot. The edge from the depot to the
            # first client is given an arrow head to indicate route direction.
            # We don't do this for the edge returning to the depot because that
            # adds a lot of clutter at the depot.
            start_loc = depot_locs[start]
            end_loc = depot_locs[end]

            kwargs = dict(linewidth=0.25, color="grey")
            ax.plot(
                [x[-1], x_coords[end_loc]],
                [y[-1], y_coords[end_loc]],
                linewidth=0.25,
                color="grey",
            )
            ax.annotate(
                "",
                xy=(x[0], y[0]),
                xytext=(x_coords[start_loc], y_coords[start_loc]),
                arrowprops=dict(arrowstyle="-|>", **kwargs),
                zorder=1,
            )

    if plot_locations and len(solution.unplanned()) != 0:
        unplanned = []
        for activity in solution.unplanned():
            match activity.type:
                case ActivityType.CLIENT:
                    client = data.client(activity.idx)
                    unplanned.append(client.location)

                case ActivityType.PICKUP:
                    shipment = data.shipment(activity.idx)
                    unplanned.append(shipment.pickup.location)

                case ActivityType.DELIVERY:
                    shipment = data.shipment(activity.idx)
                    unplanned.append(shipment.delivery.location)

                case _:
                    continue

        x = x_coords[unplanned]
        y = y_coords[unplanned]
        ax.scatter(x, y, label="Unvisited", zorder=3, s=75, c="grey")

    ax.grid(color="grey", linestyle="solid", linewidth=0.2)

    ax.set_title("Solution")
    ax.set_aspect("equal", "datalim")
    ax.legend(frameon=False, ncol=2)

from itertools import pairwise

import matplotlib.pyplot as plt
import numpy as np

from pyvrp import Activity, ProblemData, Solution


def plot_solution(
    solution: Solution,
    data: ProblemData,
    plot_clients: bool = False,
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
    plot_clients
        Whether to plot all clients as dots. Default False, which plots only
        the solution's routes.
    ax
        Axes object to draw the plot on. One will be created if not provided.
    """
    if not ax:
        _, ax = plt.subplots()

    x_coords = np.array([loc.x for loc in data.locations()])
    y_coords = np.array([loc.y for loc in data.locations()])

    depot_locs = np.array([depot.location for depot in data.depots()], int)
    client_locs = np.array([client.location for client in data.clients()], int)

    # These are the depots, as big red stars.
    kwargs = dict(label="Depot", c="tab:red", marker="*", zorder=3, s=500)
    ax.scatter(x_coords[depot_locs], y_coords[depot_locs], **kwargs)

    colors = plt.get_cmap("tab10")
    in_solution = np.zeros(data.num_clients, dtype=bool)
    for idx, route in enumerate(solution.routes()):
        color = colors(idx % colors.N)

        activities = route.schedule()
        client_activities = filter(Activity.is_client, activities)
        depot_activities = filter(Activity.is_depot, activities)

        depots = [activity.idx for activity in depot_activities]
        clients = [activity.idx for activity in client_activities]
        in_solution[clients] = True

        trips: list[list[int]] = []  # as lists of client locations
        for activity in activities[:-1]:
            if activity.is_depot():
                trips.append([])
            else:
                trips[-1].append(client_locs[activity.idx])

        if route.num_clients() == 1 or plot_clients:
            kwargs = dict(label=f"Route {idx + 1}", zorder=3, s=75)
            locs = client_locs[clients]
            ax.scatter(x_coords[locs], y_coords[locs], **kwargs, color=color)

        for locations, (start, end) in zip(trips, pairwise(depots)):
            if len(locations) == 0:
                continue

            x = x_coords[locations]
            y = y_coords[locations]

            # Clients visited by this trip, as a line segment or single dot (in
            # case of a singleton trip). Trips of the same route share colour.
            if len(locations) == 1:
                ax.scatter(x, y, zorder=3, s=75, color=color)
            else:
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

    if plot_clients and not in_solution.all():
        # Then we also plot the unvisited clients as grey dots. This is helpful
        # in understanding solutions for problems with optional clients.
        unvisited = np.flatnonzero(~in_solution)
        x = x_coords[client_locs[unvisited]]
        y = y_coords[client_locs[unvisited]]
        ax.scatter(x, y, label="Unvisited", zorder=3, s=75, c="grey")

    ax.grid(color="grey", linestyle="solid", linewidth=0.2)

    ax.set_title("Solution")
    ax.set_aspect("equal", "datalim")
    ax.legend(frameon=False, ncol=2)

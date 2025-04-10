import matplotlib.pyplot as plt
import numpy as np

from pyvrp import ProblemData, Solution


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

    num_locs = data.num_locations
    x_coords = np.array([data.location(loc).x for loc in range(num_locs)])
    y_coords = np.array([data.location(loc).y for loc in range(num_locs)])

    # These are the depots, as big red stars.
    kwargs = dict(label="Depot", c="tab:red", marker="*", zorder=3, s=500)
    ax.scatter(
        x_coords[: data.num_depots],
        y_coords[: data.num_depots],
        **kwargs,
    )

    colors = plt.get_cmap("tab10")
    in_solution = np.zeros(data.num_locations, dtype=bool)
    for idx, route in enumerate(solution.routes()):
        color = colors(idx)
        in_solution[route] = True

        if len(route) == 1 or plot_clients:  # explicit client coordinate plot
            kwargs = dict(label=f"Route {idx + 1}", zorder=3, s=75)
            ax.scatter(x_coords[route], y_coords[route], **kwargs, color=color)

        for trip in route.trips():
            if len(trip) == 0:
                continue

            x = x_coords[trip]
            y = y_coords[trip]

            # Clients visited by this trip, as a line segment or single dot (in
            # case of a singleton trip). Trips of the same route share colour.
            if len(trip) == 1:
                ax.scatter(x, y, zorder=3, s=75, color=color)
            else:
                ax.plot(x, y, color=color)

            # Thin edges from and to the depot. The edge from the depot to the
            # first client is given an arrow head to indicate route direction.
            # We don't do this for the edge returning to the depot because that
            # adds a lot of clutter at the depot.
            start_depot = trip.start_depot()
            end_depot = trip.end_depot()

            kwargs = dict(linewidth=0.25, color="grey")
            ax.plot(
                [x[-1], x_coords[end_depot]],
                [y[-1], y_coords[end_depot]],
                linewidth=0.25,
                color="grey",
            )
            ax.annotate(
                "",
                xy=(x[0], y[0]),
                xytext=(x_coords[start_depot], y_coords[start_depot]),
                arrowprops=dict(arrowstyle="-|>", **kwargs),
                zorder=1,
            )

    if plot_clients and not in_solution[data.num_depots :].all():
        # Then we also plot the unvisited clients as grey dots. This is helpful
        # in understanding solutions for problems with optional clients.
        unvisited = np.flatnonzero(~in_solution[data.num_depots :])
        x = x_coords[data.num_depots + unvisited]
        y = y_coords[data.num_depots + unvisited]
        ax.scatter(x, y, label="Unvisited", zorder=3, s=75, c="grey")

    ax.grid(color="grey", linestyle="solid", linewidth=0.2)

    ax.set_title("Solution")
    ax.set_aspect("equal", "datalim")
    ax.legend(frameon=False, ncol=2)

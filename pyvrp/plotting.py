from typing import List, Optional

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import LineCollection

from pyvrp import Individual, ProblemData
from pyvrp.Result import Result
from pyvrp.exceptions import StatisticsNotCollectedError


def plot_instance(data: ProblemData, fig: Optional[plt.Figure] = None):
    """
    Plots coordinates, time windows and demands for an instance.

    Parameters
    ----------
    data
        Data instance for which to plot coordinates, time windows and demands.
    fig, optional
        Optional Figure to draw on. One will be created if not provided.
    """
    if not fig:
        fig = plt.figure(figsize=(20, 12))

    # Uses a GridSpec instance to lay-out all subplots nicely. There are
    # two columns: left and right. Left has three rows, each containing a
    # plot with statistics: the first plots population diversity, the
    # second subpopulation objective information, and the third iteration
    # runtimes. The right column plots the solution on top of the instance
    # data.
    gs = fig.add_gridspec(2, 2, width_ratios=(2 / 5, 3 / 5))

    plot_timewindows(data, ax=fig.add_subplot(gs[0, 0]))
    plot_demands(data, ax=fig.add_subplot(gs[1, 0]))
    plot_coordinates(data, ax=fig.add_subplot(gs[:, 1]))

    plt.tight_layout()


def plot_coordinates(
    data: ProblemData,
    title: str = "Coordinates",
    ax: Optional[plt.Axes] = None,
):
    """
    Plots coordinates for clients and depot.

    Parameters
    ----------
    data
        Data instance
    title, optional
        Title to add to the plot
    ax, optional
        Axes object to draw the plot on. One will be created if not
        provided.
    """
    if not ax:
        _, ax = plt.subplots()

    dim = data.num_clients + 1
    x_coords = np.array([data.client(client).x for client in range(dim)])
    y_coords = np.array([data.client(client).y for client in range(dim)])

    # This is the depot
    kwargs = dict(c="tab:red", marker="*", zorder=3, s=500)
    ax.scatter(x_coords[0], y_coords[0], label="Depot", **kwargs)

    ax.scatter(x_coords[1:], y_coords[1:], s=75, label="Clients")

    ax.grid(color="grey", linestyle="solid", linewidth=0.2)

    ax.set_title(title)
    ax.set_aspect("equal", "datalim")
    ax.legend(frameon=False, ncol=2)


def plot_timewindows(
    data: ProblemData,
    title: str = "Time windows",
    ax: Optional[plt.Axes] = None,
):
    """
    Plots time windows for clients, as vertical bars sorted by time window.

    Parameters
    ----------
    data
        Data instance
    title, optional
        Title to add to the plot
    ax, optional
        Axes object to draw the plot on. One will be created if not
        provided.
    """
    if not ax:
        _, ax = plt.subplots()

    dim = data.num_clients + 1
    tw = np.array(
        [
            [data.client(client).tw_early, data.client(client).tw_late]
            for client in range(dim)
        ]
    )
    # Lexicographic sort so for equal start we get shorter TW first
    tw = tw[np.lexsort((tw[:, 1], tw[:, 0]))]

    lines = [((i, early), (i, late)) for i, (early, late) in enumerate(tw)]
    ax.add_collection(LineCollection(lines, linewidths=1))
    ax.set_xlim([0, dim])
    ax.set_ylim([tw.min(), tw.max()])

    ax.set_title(title)
    ax.set_xlabel("Client (sorted by TW)")
    ax.set_ylabel("Time window")


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
        Data instance
    title, optional
        Title to add to the plot
    ax, optional
        Axes object to draw the plot on. One will be created if not
        provided.
    """
    if not ax:
        _, ax = plt.subplots()

    dim = data.num_clients + 1
    # Exclude depot
    demand = np.array([data.client(client).demand for client in range(1, dim)])
    demand = np.sort(demand)

    ax.bar(np.arange(1, dim), demand)

    if title is None:
        title = (
            f"Demands (cap = {data.vehicle_capacity}, "
            + "{data.vehicle_capacity / demand.mean():.2f} stops/route)"
        )
    ax.set_title(title)
    ax.set_xlabel("Client (sorted by demand)")
    ax.set_ylabel("Demand")


def plot_result(
    result: Result, data: ProblemData, fig: Optional[plt.Figure] = None
):
    """
    Plots the results of a run: the best solution, and detailed
    statistics about the algorithm's performance.

    Parameters
    ----------
    result
        Result to be plotted.
    data
        Data instance underlying the result's solution.
    fig, optional
        Optional Figure to draw on. One will be created if not provided.

    Raises
    ------
    StatisticsNotCollectedError
        Raised when statistics have not been collected.
    """
    if not result.has_statistics():
        raise StatisticsNotCollectedError(
            "Statistics have not been collected."
        )

    if not fig:
        fig = plt.figure(figsize=(20, 12))

    # Uses a GridSpec instance to lay-out all subplots nicely. There are
    # two columns: left and right. Left has three rows, each containing a
    # plot with statistics: the first plots population diversity, the
    # second subpopulation objective information, and the third iteration
    # runtimes. The right column plots the solution on top of the instance
    # data.
    gs = fig.add_gridspec(3, 2, width_ratios=(2 / 5, 3 / 5))

    ax_diversity = fig.add_subplot(gs[0, 0])
    plot_diversity(result, ax=ax_diversity)
    plot_objectives(result, ax=fig.add_subplot(gs[1, 0], sharex=ax_diversity))
    plot_runtimes(result, ax=fig.add_subplot(gs[2, 0], sharex=ax_diversity))
    plot_solution(result.best, data, ax=fig.add_subplot(gs[:, 1]))

    plt.tight_layout()


def plot_diversity(result: Result, ax: Optional[plt.Axes] = None):
    """
    Plots population diversity statistics.

    Parameters
    ----------
    result
        Result for which to plot diversity.
    ax, optional
        Axes object to draw the plot on. One will be created if not
        provided.

    Raises
    ------
    StatisticsNotCollectedError
        Raised when statistics have not been collected.
    """
    if not result.has_statistics():
        raise StatisticsNotCollectedError(
            "Statistics have not been collected."
        )

    if not ax:
        _, ax = plt.subplots()

    ax.set_title("Diversity")
    ax.set_xlabel("Iteration (#)")
    ax.set_ylabel("Avg. diversity")

    x = 1 + np.arange(result.num_iterations)
    y = [d.avg_diversity for d in result.stats.feas_stats]
    ax.plot(x, y, label="Feas. diversity", c="tab:green")

    y = [d.avg_diversity for d in result.stats.infeas_stats]
    ax.plot(x, y, label="Infeas. diversity", c="tab:red")

    ax.legend(frameon=False)


def plot_objectives(
    result: Result,
    num_to_skip: Optional[int] = None,
    ax: Optional[plt.Axes] = None,
):
    """
    Plots each subpopulation's objective values.

    Parameters
    ----------
    result
        Result for which to plot objectives.
    num_to_skip, optional
        Number of initial iterations to skip in the plot. These early
        iterations typically have very high objective values, and obscure
        what's going on later in the search. The default skips the first 5%
        of iterations.
    ax, optional
        Axes object to draw the plot on. One will be created if not
        provided.

    Raises
    ------
    StatisticsNotCollectedError
        Raised when statistics have not been collected.
    """
    if not result.has_statistics():
        raise StatisticsNotCollectedError(
            "Statistics have not been collected."
        )

    if not ax:
        _, ax = plt.subplots()

    if num_to_skip is None:
        num_to_skip = int(0.05 * result.num_iterations)

    def _plot(x, y, *args, **kwargs):
        ax.plot(x[num_to_skip:], y[num_to_skip:], *args, **kwargs)

    x = 1 + np.arange(result.num_iterations)
    y = [d.best_cost for d in result.stats.infeas_stats]
    _plot(x, y, label="Infeas. best", c="tab:red")

    y = [d.avg_cost for d in result.stats.infeas_stats]
    _plot(x, y, label="Infeas. avg.", c="tab:red", alpha=0.3, ls="--")

    y = [d.best_cost for d in result.stats.feas_stats]
    _plot(x, y, label="Feas. best", c="tab:green")

    y = [d.avg_cost for d in result.stats.feas_stats]
    _plot(x, y, label="Feas. avg.", c="tab:green", alpha=0.3, ls="--")

    ax.set_title("Feasible objectives")
    ax.set_xlabel("Iteration (#)")
    ax.set_ylabel("Objective")

    ax.legend(frameon=False)


def plot_solution(
    solution: Individual,
    data: ProblemData,
    plot_customers: bool = False,
    ax: Optional[plt.Axes] = None,
):
    """
    Plots the best solution.

    Parameters
    ----------
    solution
        Solution to plot.
    data
        Data instance underlying the solution.
    plot_customers
        Whether to plot customers as dots (default plots only routes)
    ax, optional
        Axes object to draw the plot on. One will be created if not
        provided.
    """
    if not ax:
        _, ax = plt.subplots()

    dim = data.num_clients + 1
    x_coords = np.array([data.client(client).x for client in range(dim)])
    y_coords = np.array([data.client(client).y for client in range(dim)])

    # This is the depot
    kwargs = dict(c="tab:red", marker="*", zorder=3, s=500)
    ax.scatter(x_coords[0], y_coords[0], label="Depot", **kwargs)

    for idx, route in enumerate(solution.get_routes(), 1):
        if route:
            x = x_coords[route]
            y = y_coords[route]

            # Coordinates of customers served by this route.
            if len(route) == 1 or plot_customers:
                ax.scatter(x, y, label=f"Route {idx}", zorder=3, s=75)
            ax.plot(x, y)

            # Edges from and to the depot, very thinly dashed.
            kwargs = dict(ls=(0, (5, 15)), linewidth=0.25, color="grey")
            ax.plot([x_coords[0], x[0]], [y_coords[0], y[1]], **kwargs)
            ax.plot([x[-1], x_coords[0]], [y[-1], y_coords[0]], **kwargs)

    ax.grid(color="grey", linestyle="solid", linewidth=0.2)

    ax.set_title("Solution")
    ax.set_aspect("equal", "datalim")
    ax.legend(frameon=False, ncol=2)


def plot_runtimes(result: Result, ax: Optional[plt.Axes] = None):
    """
    Plots iteration runtimes.

    Parameters
    ----------
    result
        Result for which to plot runtimes.
    ax, optional
        Axes object to draw the plot on. One will be created if not
        provided.

    Raises
    ------
    StatisticsNotCollectedError
        Raised when statistics have not been collected.
    """
    if not result.has_statistics():
        raise StatisticsNotCollectedError(
            "Statistics have not been collected."
        )

    if not ax:
        _, ax = plt.subplots()

    x = 1 + np.arange(result.num_iterations)
    ax.plot(x, result.stats.runtimes)

    if result.num_iterations > 1:  # need data to plot a trendline
        b, c = np.polyfit(x, result.stats.runtimes, 1)  # noqa
        ax.plot(b * x + c)

    ax.set_xlim(left=0)

    ax.set_xlabel("Iteration (#)")
    ax.set_ylabel("Runtime (s)")
    ax.set_title("Iteration runtimes")


def plot_route_schedule(
    data: ProblemData,
    route: List[int],
    legend: bool = True,
    title: Optional[str] = None,
    ax: Optional[plt.Axes] = None,
):
    """
    Plots a route schedule. This function plots multiple time statistics
    as a function of distance traveled:

    Solid: earliest possible trajectory / time (using timewarp if infeasible)
    Shaded: slack up to latest possible trajectory / time (only if no timewarp)
    Dash-dotted: driving + service time, excluding wait time and timewarp
    Dotted: driving time only
    Grey shaded background: remaining load in vehicle (on secondary y-axis)

    Parameters
    ----------
    data
        Data instance for which to plot the route schedule.
    route
        Route (list of indices) to plot schedule for.
    legend
        Whether or not to show the legends
    title, optional
        Title to add to the plot
    ax, optional
        Axes object to draw the plot on. One will be created if not
        provided.
    """
    if not ax:
        _, ax = plt.subplots()

    depot = data.client(0)  # For readability, define variable
    horizon = depot.tw_late - depot.tw_early
    start_time = max(
        [depot.tw_early] + [data.client(idx).release_time for idx in route]
    )
    # Interpret depot.serv_dur as loading duration, typically 0

    # Initialize tracking variables
    t = start_time
    wait_time = 0
    time_warp = 0
    drive_time = 0
    serv_time = 0
    dist = 0
    load = sum([data.client(idx).demand for idx in route])
    slack = horizon

    # Traces and objects used for plotting
    trace_time = []
    trace_drive = []
    trace_drive_serv = []
    trace_load = []
    timewindow_lines = []
    timewindow_colors = []
    timewarp_lines = []

    def add_traces(dist, t, drive_time, serv_time, load):
        trace_time.append((dist, t))
        trace_drive.append((dist, drive_time))
        trace_drive_serv.append((dist, drive_time + serv_time))
        trace_load.append((dist, load))

    add_traces(dist, t, drive_time, serv_time, load)

    t += depot.serv_dur
    serv_time += depot.serv_dur

    add_traces(dist, t, drive_time, serv_time, load)

    prev_idx = 0  # depot
    for idx in list(route) + [0]:
        stop = data.client(idx)
        # Currently time = distance (i.e. assume speed of 1)
        delta_time = delta_dist = data.dist(prev_idx, idx)
        t += delta_time
        drive_time += delta_time
        dist += delta_dist

        add_traces(dist, t, drive_time, serv_time, load)

        if t < stop.tw_early:
            wait_time += stop.tw_early - t
            t = stop.tw_early

        slack = min(slack, stop.tw_late - t)
        if t > stop.tw_late:
            time_warp += t - stop.tw_late
            timewarp_lines.append(((dist, t), (dist, stop.tw_late)))
            t = stop.tw_late

        load -= stop.demand

        add_traces(dist, t, drive_time, serv_time, load)

        if idx != 0:  # Don't plot service and timewindow for return to depot
            t += stop.serv_dur
            serv_time += stop.serv_dur

            add_traces(dist, t, drive_time, serv_time, load)

            timewindow_lines.append(
                ((dist, stop.tw_early), (dist, stop.tw_late))
            )
            timewindow_colors.append(
                "black"
                if (stop.tw_late - stop.tw_early) <= horizon / 2
                else "gray"
            )

        prev_idx = idx

    # Plot primary traces
    xs, ys = zip(*trace_time)
    ax.plot(xs, ys, label="Time (earliest)")
    if slack > 0:
        ax.fill_between(
            xs, ys, np.array(ys) + slack, alpha=0.3, label="Time (slack)"
        )

    ax.plot(
        *zip(*trace_drive_serv), linestyle="-.", label="Drive + service time"
    )
    ax.plot(*zip(*trace_drive), linestyle=":", label="Drive time")

    # Plot time windows & time warps
    lc_time_windows = LineCollection(
        timewindow_lines,
        colors=timewindow_colors,
        linewidths=2,
        label="Time window",
    )
    ax.add_collection(lc_time_windows)
    lc_timewarps = LineCollection(
        timewarp_lines,
        colors="red",
        linewidths=2,
        alpha=0.7,
        label="Time warp",
    )
    ax.add_collection(lc_timewarps)
    ax.set_ylim(
        [depot.tw_early, max(depot.tw_late, max(t for d, t in trace_time))]
    )

    # Plot remaining load on second axis
    twin1 = ax.twinx()
    twin1.fill_between(
        *zip(*trace_load), color="black", alpha=0.1, label="Load in vehicle"
    )
    twin1.set_ylim([0, data.vehicle_capacity])

    # Set labels, legends and title
    ax.set_xlabel("Distance")
    ax.set_ylabel("Time")
    twin1.set_ylabel(f"Load (capacity = {data.vehicle_capacity})")

    if legend:
        twin1.legend(loc="upper right")
        ax.legend(loc="upper left")

    if title:
        ax.set_title(title)

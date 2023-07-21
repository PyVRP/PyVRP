from typing import Optional

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import LineCollection

from pyvrp import ProblemData, Route


def plot_route_schedule(
    data: ProblemData,
    route: Route,
    legend: bool = True,
    title: Optional[str] = None,
    ax: Optional[plt.Axes] = None,
):
    """
    Plots a route schedule. This function plots multiple time statistics
    as a function of distance travelled:

    * Solid: earliest possible trajectory / time, using time warp if the route
      is infeasible.
    * Shaded: slack up to latest possible trajectory / time, only if no time
      warp on the route.
    * Dash-dotted: driving and service time, excluding wait time and time warp.
    * Dotted: pure driving time.
    * Grey shaded background: remaining load in the vehicle.

    Parameters
    ----------
    data
        Data instance for which to plot the route schedule.
    route
        Route (list of clients) whose schedule to plot.
    legend, optional
        Whether or not to show the legends. Default True.
    title, optional
        Title to add to the plot.
    ax, optional
        Axes object to draw the plot on. One will be created if not provided.
    """
    if not ax:
        _, ax = plt.subplots()

    vehicle_type = data.vehicle_type(route.vehicle_type())
    depot = data.client(vehicle_type.depot)
    horizon = depot.tw_late - depot.tw_early

    # Initialise tracking variables
    t = route.release_time()
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
    timewarp_lines = []

    def add_traces(dist, t, drive_time, serv_time, load):
        trace_time.append((dist, t))
        trace_drive.append((dist, drive_time))
        trace_drive_serv.append((dist, drive_time + serv_time))
        trace_load.append((dist, load))

    add_traces(dist, t, drive_time, serv_time, load)

    t += depot.service_duration
    serv_time += depot.service_duration

    add_traces(dist, t, drive_time, serv_time, load)

    prev_idx = vehicle_type.depot
    for idx in [*list(route), vehicle_type.depot]:
        stop = data.client(idx)
        delta_time = data.duration(prev_idx, idx)
        delta_dist = data.dist(prev_idx, idx)
        t += delta_time
        drive_time += delta_time
        dist += delta_dist

        add_traces(dist, t, drive_time, serv_time, load)

        if t < stop.tw_early:
            t = stop.tw_early

        slack = min(slack, stop.tw_late - t)
        if t > stop.tw_late:
            timewarp_lines.append(((dist, t), (dist, stop.tw_late)))
            t = stop.tw_late

        load -= stop.demand

        add_traces(dist, t, drive_time, serv_time, load)

        if idx != vehicle_type.depot:  # exclude return to depot
            t += stop.service_duration
            serv_time += stop.service_duration

            add_traces(dist, t, drive_time, serv_time, load)

            timewindow_lines.append(
                ((dist, stop.tw_early), (dist, stop.tw_late))
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
        colors="grey",
        linewidths=1,
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
    twin1.set_ylim([0, vehicle_type.capacity])

    # Set labels, legends and title
    ax.set_xlabel("Distance")
    ax.set_ylabel("Time")
    twin1.set_ylabel(f"Load (capacity = {vehicle_type.capacity:.0f})")

    if legend:
        twin1.legend(loc="upper right")
        ax.legend(loc="upper left")

    if title:
        ax.set_title(title)

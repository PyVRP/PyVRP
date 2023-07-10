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
    ax1: Optional[plt.Axes] = None,
    ax2: Optional[plt.Axes] = None,
):
    """
    Plots a route schedule. This function plots multiple time statistics
    as a function of distance traveled:

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
    ax1, optional
        Axes object to draw the plot on. One will be created if not provided.
    ax2, optional
        Axes object to draw the plot on. One will be created if not provided.
    """
    if not ax1 and not ax2:
        _, (ax1, ax2) = plt.subplots(nrows=2, ncols=1)


    depot = data.client(0)  # For readability, define variable
    horizon = depot.tw_late - depot.tw_early

    # Initialise tracking variables
    t = 0
    wait_time = 0
    time_warp = 0
    drive_time = 0
    serv_time = 0
    dist = 0
    weight_load = sum([data.client(idx).weight_demand for idx in route])
    volume_load = sum([data.client(idx).volume_demand for idx in route])
    salvage_load = sum([data.client(idx).salvage_demand for idx in route])
    slack = horizon

    # Traces and objects used for plotting
    trace_time = []
    trace_drive = []
    trace_drive_serv = []
    trace_weight_load = []
    trace_volume_load = []
    trace_salvage_load = []
    timewindow_lines = []
    timewarp_lines = []

    def add_traces(dist, t, drive_time, serv_time, weight_load, volume_load, salvage_load):
        trace_time.append((dist, t))
        trace_drive.append((dist, drive_time))
        trace_drive_serv.append((dist, drive_time + serv_time))
        trace_weight_load.append((dist, weight_load))
        trace_volume_load.append((dist, volume_load))
        trace_salvage_load.append((dist, salvage_load))

    add_traces(dist, t, drive_time, serv_time, weight_load, volume_load, salvage_load)

    t += depot.service_duration
    serv_time += depot.service_duration

    add_traces(dist, t, drive_time, serv_time, weight_load, volume_load, salvage_load)

    prev_idx = 0  # depot
    for idx in list(route) + [0]:
        stop = data.client(idx)
        delta_time = data.duration(prev_idx, idx)
        delta_dist = data.dist(prev_idx, idx)
        t += delta_time
        drive_time += delta_time
        dist += delta_dist

        add_traces(dist, t, drive_time, serv_time, weight_load, volume_load, salvage_load)

        if t < stop.tw_early:
            wait_time += stop.tw_early - t
            t = stop.tw_early

        slack = min(slack, stop.tw_late - t)
        if t > stop.tw_late:
            time_warp += t - stop.tw_late
            timewarp_lines.append(((dist, t), (dist, stop.tw_late)))
            t = stop.tw_late

        weight_load -= stop.weight_demand
        volume_load -= stop.volume_demand
        salvage_load -= stop.salvage_demand

        add_traces(dist, t, drive_time, serv_time, weight_load, volume_load, salvage_load)

        if idx != 0:  # Don't plot service and timewindow for return to depot
            t += stop.service_duration
            serv_time += stop.service_duration

            add_traces(dist, t, drive_time, serv_time, weight_load, volume_load, salvage_load)

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

    # Plot remaining weight load on second axis
    twin1 = ax[0].twinx()
    twin1.fill_between(
        *zip(*trace_weight_load), color="black", alpha=0.1, label="Weight load in vehicle"
    )
    twin1.set_ylim([0, data.weight_capacity])
    
    # Plot remaining volume load on third axis
    twin2 = ax[1].twinx()
    twin2.fill_between(
        *zip(*trace_volume_load), color="blue", alpha=0.1, label="Volume load in vehicle"
    )
    twin2.set_ylim([0, data.volume_capacity])

    # Plot remaining salvage load on third axis
    twin3 = ax[1].twinx()
    twin3.fill_between(
        *zip(*trace_salvage_load), color="blue", alpha=0.1, label="Volume load in vehicle"
    )
    twin3.set_ylim([0, data.salvage_capacity])
    
    # Set labels for both axes
    twin1.set_ylabel(f"Weight Load (weight_capacity = {data.weight_capacity:.0f})")
    twin2.set_ylabel(f"Volume Load (volume_capacity = {data.volume_capacity:.0f})")
    twin3.set_ylabel(f"Volume Load (salvage_capacity = {data.salvage_capacity:.0f})")

    if legend:
        twin1.legend(loc="upper right")
        twin2.legend(loc="upper left")
        twin2.legend(loc="upper right")
        ax[0].legend(loc="upper left")
        ax[1].legend(loc="lower left")
        ax[2].legend(loc="lower left")

    if title:
        ax[0].set_title(f"{title} - Weight Load")
        ax[1].set_title(f"{title} - Volume Load")
        ax[2].set_title(f"{title} - Salvage Load")

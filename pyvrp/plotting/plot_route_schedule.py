import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import LineCollection

from pyvrp import Client, ProblemData, Route


def plot_route_schedule(
    data: ProblemData,
    route: Route,
    load_dimension: int = 0,
    legend: bool = True,
    title: str | None = None,
    ax: plt.Axes | None = None,
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
    * Grey shaded background: remaining load in the vehicle for the provided
      load dimension.

    Parameters
    ----------
    data
        Data instance for which to plot the route schedule.
    route
        Route (list of clients) whose schedule to plot.
    load_dimension
        Load dimension to plot. Defaults to the first dimension, if it exists.
    legend
        Whether or not to show the legends. Default True.
    title
        Title to add to the plot.
    ax
        Axes object to draw the plot on. One will be created if not provided.
    """
    if not ax:
        _, ax = plt.subplots()

    vehicle_type = data.vehicle_type(route.vehicle_type())
    distances = data.distance_matrix(vehicle_type.profile)
    durations = data.duration_matrix(vehicle_type.profile)

    track_load = load_dimension < data.num_load_dimensions

    # Initialise tracking variables
    drive_time = 0
    serv_time = 0
    dist = 0

    load = 0
    if track_load:
        load = route.delivery()[load_dimension]

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

    prev_idx = vehicle_type.start_depot
    for visit in route.schedule():
        idx = visit.location
        stop = data.location(idx)

        drive_time += durations[prev_idx, idx]
        dist += distances[prev_idx, idx]

        arrive = visit.start_service - visit.wait_duration
        add_traces(dist, arrive, drive_time, serv_time, load)

        if visit.time_warp > 0:
            true_arrive = visit.start_service + visit.time_warp
            timewarp_lines.append(((dist, true_arrive), (dist, stop.tw_late)))

        if isinstance(stop, Client) and track_load:
            load -= stop.delivery[load_dimension]
            load += stop.pickup[load_dimension]

        add_traces(dist, visit.start_service, drive_time, serv_time, load)

        serv_time += visit.service_duration
        add_traces(dist, visit.end_service, drive_time, serv_time, load)

        timewindow_lines.append(((dist, stop.tw_early), (dist, stop.tw_late)))

        prev_idx = idx

    xs, ys = zip(*trace_time)
    ax.plot(xs, ys, label="Time (earliest)")

    if route.slack() > 0:
        ax.fill_between(
            xs,
            ys,
            np.array(ys) + route.slack(),
            alpha=0.3,
            label="Time (slack)",
        )

    ax.plot(
        *zip(*trace_drive_serv), linestyle="-.", label="Drive + service time"
    )
    ax.plot(*zip(*trace_drive), linestyle=":", label="Drive time")

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
        bottom=vehicle_type.tw_early,
        top=max(vehicle_type.tw_late, max(t for _, t in trace_time)),
    )

    # Plot remaining load on second axis
    if track_load:
        capacity = vehicle_type.capacity[load_dimension]

        twin1 = ax.twinx()
        twin1.fill_between(
            *zip(*trace_load),
            color="black",
            alpha=0.1,
            label="Load in vehicle",
        )

        twin1.set_ylim([0, capacity])
        twin1.set_ylabel(f"Load (capacity = {capacity:.0f})")

        if legend:
            twin1.legend(loc="upper right")

    ax.set_xlabel("Distance")
    ax.set_ylabel("Time")

    if legend:
        ax.legend(loc="upper left")

    if title:
        ax.set_title(title)

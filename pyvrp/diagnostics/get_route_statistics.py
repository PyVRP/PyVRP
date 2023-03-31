from dataclasses import dataclass
from typing import List

from pyvrp import Individual, ProblemData


@dataclass
class RouteStatistics:
    """
    Object storing various route statistics.

    Attributes
    ----------
    distance
        Total distance travelled over this route.
    start_time
        Earliest starting time on this route, that is, the earliest time this
        route can leave the depot.
    end_time
        Time at which the truck returns to the depot.
    duration
        Total route duration, including waiting time.
    timewarp
        Any time warp incurred along the route.
    wait_time
        Time spent waiting on the route.
    service_time
        Total service time on the route.
    num_stops
        Number of clients visits along the route.
    total_demand
        Total client demand on this route.
    fillrate
        Capacity used by total client demand.
    is_feasible
        Whether the route is load and time feasible.
    is_empty
        Whether the route is empty.
    """

    distance: int
    start_time: int
    end_time: int
    duration: int
    timewarp: int
    wait_time: int
    service_time: int
    num_stops: int
    total_demand: int
    capacity: int
    fillrate: float
    is_feasible: bool
    is_empty: bool


def get_route_statistics(
    data: ProblemData, route: List[int], route_idx: int = 0
) -> RouteStatistics:
    """
    Returns statistics for a route.

    Parameters
    ----------
    data
        Data instance corresponding to the route.
    route
        Route (list of indices) to compute statistics for.
    route_idx, optional
        Route idx of the route to get statistics for. Use to get the correct
        capacity with a heteregeneous fleet. Defaults to 0 (first route).

    Returns
    -------
    RouteStatistics
        Object with statistics for the route.
    """
    assert 0 not in route, "Route should not contain depot"

    depot = data.depot()  # For readability, define variable
    start_time = depot.tw_early

    # Interpret depot.service_duration as loading duration, typically 0
    current_time = start_time + depot.service_duration
    wait_time = 0
    time_warp = 0
    distance = 0

    prev_idx = 0  # depot
    for idx in list(route) + [0]:
        stop = data.client(idx)
        delta = data.dist(prev_idx, idx)
        current_time += delta
        distance += delta

        if current_time < stop.tw_early:
            wait_time += stop.tw_early - current_time
            current_time = stop.tw_early

        if current_time > stop.tw_late:
            time_warp += current_time - stop.tw_late
            current_time = stop.tw_late

        if idx != 0:  # Don't count final depot
            current_time += stop.service_duration
        prev_idx = idx

    demand = sum([data.client(idx).demand for idx in route])
    serv_dur = sum([data.client(idx).service_duration for idx in route])

    vehicle_capacity = data.route(route_idx).vehicle_capacity
    return RouteStatistics(
        distance=distance,
        start_time=start_time,
        end_time=current_time,
        duration=current_time - start_time,
        timewarp=time_warp,
        wait_time=wait_time,
        service_time=serv_dur,
        num_stops=len(route),
        total_demand=demand,
        capacity=vehicle_capacity,
        fillrate=demand / vehicle_capacity,
        is_feasible=time_warp == 0 and demand <= vehicle_capacity,
        is_empty=len(route) == 0,
    )


def get_all_route_statistics(
    solution: Individual, data: ProblemData
) -> List[RouteStatistics]:
    """
    Returns route statistics for a set of routes. Empty routes are skipped.

    Parameters
    ----------
    solution
        The solution individual for which to get all route statistics.
    data
        Data instance corresponding to the route.

    Returns
    -------
    List[RouteStatistics]
        List of RouteStatistic objects with statistics for each route.
    """
    return [
        get_route_statistics(data, route, idx)
        for idx, route in enumerate(solution.get_routes())
        if route  # skip empty routes
    ]

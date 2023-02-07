from dataclasses import dataclass
from typing import List

from pyvrp import Individual, ProblemData


@dataclass
class RouteStatistics:
    distance: int
    start_time: int
    end_time: int
    duration: int
    timewarp: int
    wait_time: int
    service_time: int
    num_stops: int
    total_demand: int
    fillrate: float
    is_feasible: bool
    is_empty: bool


def get_route_statistics(
    data: ProblemData, route: List[int]
) -> RouteStatistics:
    """
    Returns statistics for a route

    Parameters
    ----------
    data
        Data instance corresponding to the route.
    route
        Route (list of indices) to compute statistics for.

    Returns
    -------
    RouteStatistics
        Object with statistics for the route.
    """
    assert 0 not in route, "Route should not contain depot"

    depot = data.depot()  # For readability, define variable
    start_time = max(
        [depot.tw_early] + [data.client(idx).release_time for idx in route]
    )
    # Interpret depot.serv_dur as loading duration, typically 0
    current_time = start_time + depot.serv_dur
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
            current_time += stop.serv_dur
        prev_idx = idx

    demand = sum([data.client(idx).demand for idx in route])
    return RouteStatistics(
        distance=distance,
        start_time=start_time,
        end_time=current_time,
        duration=current_time - start_time,
        timewarp=time_warp,
        wait_time=wait_time,
        service_time=depot.serv_dur
        + sum([data.client(idx).serv_dur for idx in route]),
        num_stops=len(route),
        total_demand=demand,
        fillrate=demand / data.vehicle_capacity,
        is_feasible=time_warp == 0 and demand <= data.vehicle_capacity,
        is_empty=len(route) == 0,
    )


def get_all_route_statistics(
    solution: Individual, data: ProblemData
) -> List[RouteStatistics]:
    """
    Returns route statistics for a set of routes.

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
        get_route_statistics(data, route) for route in solution.get_routes()
    ]
